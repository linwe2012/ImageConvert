//Author: Leon Lin
//Github: Linwe2012@163.com
//Description: my own math class to deal with
//codes I don't want to write again.
//Will add more functions as my 
//image info process tech course goes on.
#if 0
#include<vector>
#define COUNTER_LARGE_T long long int

//namespace LMath {
	template<typename Mat_t>
	//Only Support 2D matrix, trying to support more dimensions
	class mat {
	public:
		Mat_t *matrix;
		int dims; //dimension
		COUNTER_LARGE_T space;
		std::vector<int>size;
		mat(int rows, int columns);
		mat(int depth, int rows, int columns);
		mat();
		void flush(int symbol);
		Mat_t * operator[](const int idx) const;
		//template<typename Mat_t, typename MatB_t>
		template<typename MatA_t>
		mat<Mat_t> operator*(const mat<MatA_t> &A) const;
		template<typename MatA_t>
		mat<MatA_t> duplicate();
		//mat<MatA_t>& operator=(const mat<Mat_t> &A) const;
		Mat_t operator()(std::vector<int> &idxs);
		template<typename Page_t, typename MatA_t>
		//void conv(page<Page_t> dst, mat<MatA_t> kernel, int step = 1);
		//friend mat<Mat_t> operator*(const mat<Mat_t> &A, const mat<MatB_t> &B);
		friend mat<int> operator*(const mat<Mat_t> &A, const int scaler);
	private:
		void updateSpace();
	};


	template<typename Page_t>
	class page {
	public:
		mat<Page_t> &target;
		std::vector<std::vector<int>>anchors;
		std::vector<int>pivot;
		page(mat<Page_t> &target);
		void feed(Page_t elem);
	};

#endif

//}