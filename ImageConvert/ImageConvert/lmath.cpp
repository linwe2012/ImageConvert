#include "lmath.h"
#include <memory>
#include <cmath>
#define LM_ASSERT(boolean) assert(boolean);
#define LM_ASSERT_MSG(boolean, errmsg) ({\
if(msg != NULL && !(boolean))\
{std::cout << errmsg << std::endl} \
assert(boolean); \
})
#if 0
//namespace LMath {
	template<typename Mat_t>
	mat<Mat_t>::mat(int rows, int columns) 
		:dims(2)
	{
		size.push_back(rows);
		size.push_back(columns);
		matrix = (Mat_t *)malloc(rows * columns * sizeof(Mat_t));
		updateSpace();
	}
	template<typename Mat_t>
	mat<Mat_t>::mat(int depth, int rows, int columns)
		:dims(3)
	{
		size.push_back(depth);
		size.push_back(rows);
		size.push_back(columns);
		matrix = (Mat_t *)malloc(depth * rows * columns * sizeof(Mat_t));
		updateSpace();
	}
	template<typename Mat_t>
	mat<Mat_t>::mat()
		:dims(0), matrix(NULL), space(0)
	{
	}
	template<typename Mat_t>
	void mat<Mat_t>::updateSpace() {
		space = 1;
		for (int i = 0; i < dims; i++) {
			space *= size[i];
		}
	}
	template<typename Mat_t>
	void mat<Mat_t>::flush(int symbol) {
		memset(matrix, symbol, space * sizeof(Mat_t));
	}
	template<typename Mat_t>
	Mat_t * mat<Mat_t>::operator[](const int idx) const
	{
		return &matrix[idx * size[1]];
	}
	template<typename Mat_t>
	Mat_t mat<Mat_t>::operator()(std::vector<int> &idxs)
	{
		int idx = 0;
		int postfix = 1;
		if (idxs.size() != this->dims) {
			return 0;
		}
		for (int cnt = this->dims - 1; cnt >= 0; cnt --) {
			idx += postfix * idxs[cnt];
			postfix *= this->size[cnt];
		}
		return this->matrix[idx];
	}
	template<typename Mat_t>
	template<typename MatA_t>
	mat<MatA_t> mat<Mat_t>::duplicate() {
		mat<MatA_t> res;
		res.matrix = (MatA_t *)malloc(size(MatA_t) * this->space);
		for (int i = 0; i < space; i++) //can't use memcpy
		{
			res.matrix[i] = static_cast<MatA_t>(this->matrix[i]);
		}
		res.dims = this->dims;
		res.size = this->size;
		res.space = this->space;
		return res;
	}

	template<typename Mat_t>
	template<typename MatA_t>
	mat<Mat_t> mat<Mat_t>::operator*(const mat<MatA_t> &A) const
	{
		int rows = this->size[0];
		int columns = A.size[1];
		int i, j, k;
		int elem_cnt = this->size[1];
		long double temp;
		//LM_ASSERT(elem_cnt == A.size[0]);
		mat<Mat_t> res(rows, columns);
		//TODO: Optimize with simd
		//TODO: Add broadcast feature
		for (i = 0; i < rows; i++) {
			for (j = 0; j < columns; j++) {
				temp = 0;
				for (k = 0; k < elem_cnt; k++) {
					temp += (*this)[i][k] * A[k][j];
				}
			}
		}
		return res;
	}
	
	template<typename Mat_t>
	mat<int> operator*(const mat<Mat_t> &A, const int scaler)
	{
		mat<int> res;
		
		for (int d = 0; d < A.space; d++) {
			;
		}
		return res;
	}
	template<typename Mat_t>
	template<typename Page_t, typename MatA_t>
	void mat<Mat_t>::conv(page<Page_t> &dst, mat<MatA_t> &kernel, int step) {
		std::vector<int> cnt;
		for (int i = 0; i < dims; i++) {
			cnt.push_back( floor((size[i] - kernel.size[i]) * 1.0 / step)  )
		}
	}

	//template<typename Mat_t, typename MatB_t>
	//template<typename MatB_t>
	/*
	mat<Mat_t> operator*(const mat<Mat_t> &A, const mat<MatB_t> &B) {
		int rows = A.size[0];
		int columns = B.size[1];
		int i, j, k;
		int elem_cnt = A.size[1];
		long double temp;
		//LM_ASSERT(elem_cnt == B.size[0]);
		mat<Mat_t> res(rows, columns);
		//TODO: Optimize with simd
		//TODO: Add broadcast feature
		for (i = 0; i < rows; i++) {
			for (j = 0; j < columns; j++) {
				temp = 0;
				for (k = 0; k < elem_cnt; k++) {
					temp += A[i][k] * B[k][j];
				}
				res[i][j] = static_cast<Mat_t>(temp);
			}
		}
		return res;
	}*/
//}
	template<typename Page_t>
	page<Page_t>::page(mat<Page_t> &target) {
		this->target = target;
	}
#undef COUNTER_LARGE_T

#include<iostream>
	int main() 
	{
		mat<int> smg(10, 10), uvw(10, 10);
		
		smg.flush(0);
		smg[2][9] = 10;
		std::cout << smg[2][9] << "\t" << smg[0][0]<< std::endl;
		mat<int> q = smg * uvw;

		while (true)
		{

		}
		return 0;
	}

#endif