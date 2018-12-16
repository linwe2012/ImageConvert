#ifndef _DEF_UTILS_H_
#define _DEF_UTILS_H_
#ifdef _WIN32
#include <windows.h>
#else
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int32_t LONG;
typedef uint8_t BYTE;
#endif //_WIN32



/* Part1: Image Structure
*
*/
typedef struct tagMYBITMAPFILEHEADER
{
	WORD bfType; //固定为0x4d42; 'BW'
	DWORD bfSize; //文件大小
	DWORD bfReserves;
	//WORD bfReserved1; //保留字，不考虑
	//WORD bfReserved2;  //保留字，同上
	DWORD bfOffBits; //实际位图数据的偏移字节数，即前三个部分长度之和
} MYBITMAPFILEHEADER;

typedef struct tagMYBITMAPFILEINFO
{
	DWORD biSize;//(第14、15、16、17字节)
	LONG  biWidth;// (第18、19、20、21字节)
	LONG  biHeight; //(第22、23、24、25字节)
	DWORD biPlanes_biBitCount; //combined stuff for compatibility
							   //WORD  biPlanes; // (第26、27字节)
							   //WORD  biBitCount; // (第28、29字节) 采用颜色位数，可以是1，2，4，8，16，24，新的可以是32
	DWORD biCompression; // (第30，31，32，33字节) 0x22
	DWORD biSizeImage; //(第34、35、36、37字节)
	LONG biXPelsPerMeter; // (第38、39、40、41字节)
	LONG biYPelsPerMeter; // (第42、43、44、45字节)
	DWORD biClrUsed;  // (第46、47、48、49字节)
					  //00 00 00 00 即0，位图使用颜色表中的颜色数是0，
	DWORD biClrImportant; // (第50、51、52、53字节)
						  //00 00 00 00 即 0 位图显示过程中重要的颜色数是0
} MYBITMAPFILEINFO;

typedef struct tagMYRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
}MYRGBQUAD;


typedef float image_t;

typedef struct tagImage
{
	image_t *im; //where image data is stored.
	int padded; //if image is padded.
	int status; //rgb, yuv, gray, bin and so on.
	int has_opacity;
	MYBITMAPFILEHEADER *bmpHeader;
	MYBITMAPFILEINFO *bmpInfo;
	MYRGBQUAD *plate; //color plate, by default is NULL.
	char *name;
} ImageBox;
typedef struct tagImage *Image;

typedef union tagPixel {
	struct
	{
		image_t R;
		image_t G;
		image_t B;
		image_t A;
	};
	struct
	{
		image_t Y;
		image_t U;
		image_t V;
		image_t A_0;
	};
	struct
	{
		image_t Hue;
		image_t Sat;
		image_t Val;
		image_t A_1;
	};
}Pixel;

typedef struct tagVector3 Vector3;
struct tagVector3 {
	double x;
	double y;
	double z;
};

typedef struct tagConv2ConfigBox Conv2ConfigBox;
struct tagConv2ConfigBox {
	image_t * kernel; //convolution kernel
	// kernel width & height
	int kwidth;
	int kheight;
	int step; //conv step
	int *layers; // which layers(dimension) it implemnts on.
	//bounds of top-letf and right bottom
	//conv only applied within bounds,
	//if bounds<=1, then metric will be regarded as ratio, 
	//otherwise metric is assumed as pixels
	Vector3 bounds[2];
	int skip;
};

enum {
	rgb,
	yuv,
	ycbcr,
	hsv,
	gray,
	binary,
};

enum {
	imtype_jpg,
	imtype_bmp,
	imtype_png,
	imtype_unkown,
};

typedef void(*Interpolation_Func)(Image src, Pixel *res, double x, double y, int color);
typedef struct tagSTConfig *STConfig; //Spatial Transform configuration
struct tagSTConfig {
	Interpolation_Func itp; // Interpolation Function
	//only part of image within bound will be applied with spatial trnsform
	//if bounds are < 0, then it will be considered as original image width & height
	//e.g. if bounds[0].x < 0 while bounds[1].x = 100, only from left to 100 pixels will be transformed
	Vector3 bounds[2]; 
	//default pixel if can't find a match for dst
	Pixel empty; 
	//whether it should add an alpha channel.
	//if it is set to 1, an alpha channel is added even if source image w/o one 
	//This is not mendatory for functions, so e.g. scale will ignore it
	//It only works when opacity is necessary to output, e.g. rotation.
	int has_opacity;
	//if source img has no alpha channel while dst requires one, 
	// dst's alpha channel will be set to default_alpha
	image_t default_alpha; 
};
/* Part2: Basic oprations defined by macros
*
*/
#define DEF_IMAGE_ROW_WIDTH(image, row_name, col_name, row_width_name) \
int row_name = image->bmpInfo->biHeight;\
int col_name = image->bmpInfo->biWidth;\
int row_width_name = (imcolorCnt(image) * col_name + 3) & ~3;

/* FUNC_TRAVERSE_PIXEL -- macro to traverse pixel
* typename - the type of pointer to manipulate, (i.e. [imptr])
* step - the step by !BYTES
* row_name - the row of iamge
* col_name - colum of image
* row_width - the width of a row, note that row_width 
* . . . . . - is by the type of image_data_pointer
* do_before_loop - do something before the inner loop
* do_somthing - do somthing in the loop
*/
#define FUNC_TRAVERSE_PIXEL(image_data_pointer, type_name, step, row_name, col_name, row_witdh_name, do_before_loop, do_somthing) do{ \
	int i, j; \
	type_name *imptr = NULL; \
	for (i = 0; i<row_name; i++) { \
		imptr = (type_name *)(image_data_pointer + (row_witdh_name) * i); \
		do_before_loop;\
		for (j = 0; j<col_name; j++) { \
			do_somthing; \
			imptr = (type_name *)((char *)imptr + step); \
		} \
	} \
}while(0);

#define FUNC_TUPLE_ASSIGN_3(A_SRC, B_SRC, C_SRC)do{\
	A = A_SRC;\
	B = B_SRC;\
	C = C_SRC;\
	}while(0);

#define FUNC_DO_NOTHING {}

#define VAR_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define VAR_MIN(a, b) (((a) < (b)) ? (a) : (b))

#ifdef __cplusplus
#define __CAST_KERNAL(type, x) static_cast<type>(x)
#else
#define __CAST_KERNAL(type, x) (type)(x)
#endif // __cplusplus

#define CAST_INT(x) __CAST_KERNAL(int, (x))
#define CAST_UINT8_T(x) __CAST_KERNAL(uint8_t, (x))
#define CAST_IMAGE_T(x) __CAST_KERNAL(image_t, (x))
#if _MSC_VER
inline uint8_t CAST_UINT8_ENFORCE_RANGE(image_t x) {
	if (x > 255) return 255U;
	else if (x < 0) return 0U;
	return __CAST_KERNAL(uint8_t, x);
}
#elif _GNU_C
#endif

#define SPATIAL_DIMS 3

/*small micros return pointer to image/matrix according coordination.
* row_xxx_width must defined to tell micros the width of a image.
*/
#define IMAGE_POINTER(img, row, col) (img->im + (row)* row_##img##_width + col)
#define MAT2D_POINTER(mat, row, col) (mat + (row)* row_##mat##_width + col)


/* Part2: Necessary function
*
*/
// FILE *GLIM_Global_Output_Specify; //by default is adding to stdout
#define GLIM_Global_Output_Specify stdout
#define GLIM_LOG_OUTPUT(...) fprintf(GLIM_Global_Output_Specify, __VA_ARGS__)
#define GLIM_LOG_OUTPUT_V(...) vfprintf(GLIM_Global_Output_Specify, __VA_ARGS__)

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif // __cplusplus

EXTERNC void throwWarn(const char *s, ...);
EXTERNC void throwNote(const char *s, ...);

EXTERNC Image newImage();
EXTERNC Image destroyImage(Image im);

EXTERNC int imcolorCnt(Image src);
EXTERNC int imsize(Image src);

EXTERNC Image resizeImage(Image dst, int size);
EXTERNC Image duplicateImage(Image dst, Image src);
EXTERNC void createPlate(Image dst);
EXTERNC void copyImageInfo(Image dst, Image src);
EXTERNC void setBmpInfo(Image src, const char* fmt, ...);
#undef EXTERNC
#endif // _DEF_UTILS_H_