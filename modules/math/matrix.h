#pragma once

#define MAX_MATRIX_ELEMENTS 256

class matrix
{
public:
	int m;		// m rows
	int n;		// n columns
	float data[MAX_MATRIX_ELEMENTS];		// m * n matrix, row first

	matrix(int m, int n, float data[]);
	matrix();
	matrix(int d);				// d*d identity matrix
	matrix(const matrix&v);		// copy constructor
	~matrix();

	void operator =(const matrix &v);
	void operator +=(const matrix &v);
	matrix operator +(const matrix &v);
	void operator -=(const matrix &v);
	matrix operator -(const matrix &v);
	void operator *=(const matrix &v);
	matrix operator *(const matrix &v);
	void operator *=(const float &v);
	matrix operator *(const float &v);
	void operator /=(const float &v);
	void operator /=(const matrix &v);
	matrix operator /(const matrix &v);
	matrix operator /(const float &v);
	matrix inverse();
	float det();
	matrix cofactor(int a, int b);
	void identity();

private:
	float det2x2();
	float det3x3();
};