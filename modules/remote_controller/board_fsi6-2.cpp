#include "board.h"

#include <HAL/STM32F1/F1SPI.h>
#include <HAL/STM32F1/F1GPIO.h>
#include <HAL/STM32F1/F1Interrupt.h>
#include <misc.h>
#include <HAL/STM32F1/F1Timer.h>
#include <HAL/Interface/II2C.h>
#include <HAL/Interface/ISysTimer.h>
#include <stm32f10x.h>
#include <string.h>

using namespace STM32F1;
using namespace HAL;

HAL::ISPI *spi;
HAL::IGPIO *cs;
HAL::IGPIO *ce;
HAL::IGPIO *irq;
HAL::IGPIO *dbg;
HAL::IGPIO *dbg2;
HAL::IGPIO *SCL;
HAL::IGPIO *SDA;
HAL::IInterrupt *interrupt;
HAL::ITimer *timer;

int16_t adc_data[6] = {0};
namespace sheet1
{
	F1GPIO cs(GPIOC, GPIO_Pin_6);
	F1GPIO ce(GPIOC, GPIO_Pin_7);
	F1GPIO irq(GPIOB, GPIO_Pin_12);
	F1GPIO dbg(GPIOB, GPIO_Pin_10);
	
	F1GPIO dbg2(GPIOB, GPIO_Pin_11);
	F1GPIO SCL(GPIOC, GPIO_Pin_13);
	F1GPIO SDA(GPIOC, GPIO_Pin_14);
	
	F1SPI spi;
	F1Interrupt interrupt;
	F1Timer timer(TIM2);
	
	F1GPIO pa6(GPIOA, GPIO_Pin_6);
	F1GPIO pc8(GPIOC, GPIO_Pin_8);

	
	
	static void ADC1_Mode_Config(void)
	{
		ADC_InitTypeDef ADC_InitStructure;
		GPIO_InitTypeDef GPIO_InitStructure;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
		
		// Configure GPIO0~2 as analog input
		for(int ADC_Channel=0; ADC_Channel<7; ADC_Channel++)
		{
			GPIO_InitStructure.GPIO_Pin = (1 << (ADC_Channel%8));
			GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
			GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
			GPIO_Init(ADC_Channel>8?GPIOB:GPIOA, &GPIO_InitStructure);
		}

		
		DMA_InitTypeDef DMA_InitStructure;
		
		/* DMA channel1 configuration */
		DMA_DeInit(DMA1_Channel1);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);	 //ADC地址
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&adc_data;//内存地址
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
		DMA_InitStructure.DMA_BufferSize = 6;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设地址固定
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //内存地址固定
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//半字
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;		//循环传输
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
		DMA_Init(DMA1_Channel1, &DMA_InitStructure);
		
		/* Enable DMA channel1 */
		DMA_Cmd(DMA1_Channel1, ENABLE);
		
		/* ADC1 configuration */		
		ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//独立ADC模式
		ADC_InitStructure.ADC_ScanConvMode = ENABLE ; 	 //禁止扫描模式，扫描模式用于多通道采集
		ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;	//开启连续转换模式，即不停地进行ADC转换
		ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//不使用外部触发转换
		ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 	//采集数据右对齐
		ADC_InitStructure.ADC_NbrOfChannel = 6;	 	//要转换的通道数目1
		ADC_Init(ADC1, &ADC_InitStructure);
		
		/*配置ADC时钟，为PCLK2的8分频，即9Hz*/
		RCC_ADCCLKConfig(RCC_PCLK2_Div8); 
		/*配置ADC1的通道11为55.	5个采样周期，序列为1 */ 

// channel map:
// PA0		throttle
// PA1		rudder
// PA2		roll
// PA4		pitch
// PA5		left switch
// PA6		right switch

		ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_239Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 2, ADC_SampleTime_239Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 3, ADC_SampleTime_239Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 4, ADC_SampleTime_239Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 5, ADC_SampleTime_239Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 6, ADC_SampleTime_239Cycles5);
		
		/* Enable ADC1 DMA */
		ADC_DMACmd(ADC1, ENABLE);
		
		/* Enable ADC1 */
		ADC_Cmd(ADC1, ENABLE);
		
		/*复位校准寄存器 */   
		ADC_ResetCalibration(ADC1);
		/*等待校准寄存器复位完成 */
		while(ADC_GetResetCalibrationStatus(ADC1));
		
		/* ADC校准 */
		ADC_StartCalibration(ADC1);
		/* 等待校准完成*/
		while(ADC_GetCalibrationStatus(ADC1));
		
		/* 由于没有采用外部触发，所以使用软件触发ADC转换 */ 
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	}

	int sheet1_init()
	{
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
		
		::cs = &cs;
		::ce = &ce;
		::irq = &irq;
		::dbg = &dbg;
		::dbg2 = &dbg2;
		::SCL = &SCL;
		::SDA = &SDA;
		::spi = &spi;
		::interrupt = &interrupt;
		::timer = &timer;
		::bind_button = &pc8;
		pc8.set_mode(MODE_IN);
		
		spi.init(SPI2);
		interrupt.init(GPIOB, GPIO_Pin_12, interrupt_falling);
		
		pa6.set_mode(MODE_IN);
		
		ADC1_Mode_Config();
		
		return 0;
	}

	
	extern "C" void TIM2_IRQHandler(void)
	{
		timer.call_callback();
	}
}

using namespace sheet1;

uint8_t reg[10];

extern "C" void SystemCoreClockUpdate();
extern "C" void SystemInit();

int board_init()
{
	sheet1_init();
	
	int64_t up = systimer->gettime();
	F1GPIO qon(GPIOB, GPIO_Pin_2);
	qon.set_mode(MODE_IN);
	::dbg->write(false);
	::dbg2->write(false);
	::dbg->set_mode(MODE_OUT_OpenDrain);
	::dbg2->set_mode(MODE_OUT_OpenDrain);
	
	I2C_SW i2c(::SCL, ::SDA);
	
	i2c.set_speed(10);
	for(int i=0; i<10; )
	{
		if (0 == i2c.read_reg(0x6b<<1, i, reg+i))
			i++;
		else
		{
			::SCL->write(false);
			::SCL->set_mode(MODE_OUT_PushPull);
			systimer->delayus(10);
			::SDA->write(false);
			::SDA->set_mode(MODE_OUT_PushPull);
			systimer->delayus(10);
			::SCL->write(true);
			systimer->delayus(10);
			::SDA->write(true);
			::SCL->set_mode(MODE_OUT_OpenDrain);
			::SDA->set_mode(MODE_OUT_OpenDrain);

		}
		
		::dbg->toggle();
	}
	
	SystemInit();
	SystemCoreClockUpdate();
	
	if (SystemCoreClock != 72000000)
	{
		for(int i=0; i<10; i++)
		{
			::dbg->toggle();
			systimer->delayms(50);
			::dbg->toggle();
			systimer->delayms(50);
		}
		NVIC_SystemReset();
	}
	
	//reg[1] = (reg[1] & 0xcf) | 0x30;
	//i2c.write_reg(0x6b<<1, 1, reg[1]);
	
	while(1)
	{
		if (!qon.read())
			up = systimer->gettime();
		
		if (systimer->gettime() > up + 500000)
		{
			reg[5] &= 0xcf;
			i2c.write_reg(0x6b<<1, 5, reg[5]);
			reg[7] |= 0x20;			
			i2c.write_reg(0x6b<<1, 7, reg[7]);
			
			NVIC_SystemReset();
		}
	}
	
	
	return 0;	
}



void read_channels(int16_t *channel, int max_channel_count)
{
	if (max_channel_count > sizeof(adc_data)/2)
		max_channel_count = sizeof(adc_data)/2;
	
	memcpy(channel, adc_data, max_channel_count * 2);	
}