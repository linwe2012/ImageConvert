#ifndef _DEF_UTILS_H_
#define _DEF_UTILS_H_
#ifdef _WIN32
#include <windows.h>
#else
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int32_t LONG;
typedef uint8_t BYTE;
#endif
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
	};
	struct
	{
		image_t Y;
		image_t U;
		image_t V;
	};
	struct
	{
		image_t Hue;
		image_t Sat;
		image_t Val;
	};
}Pixel;

enum {
	rgb,
	yuv,
	ycbcr,
	hsv,
	gray,
	binary,
};

/* Part2: Basic oprations defined by macros
*
*/
#define DEF_IMAGE_ROW_WIDTH(image, row_name, col_name, row_width_name) \
int row_name = image->bmpInfo->biHeight;\
int col_name = image->bmpInfo->biWidth;\
int row_width_name = (imcolorCnt(image) * col_name + 3) & ~3;

#define FUNC_TRAVERSE_PIXEL(image_data_pointer, type_name, row_name, col_name, row_witdh_name, do_somthing) do{ \
	int i, j; \
	type_name *imptr = NULL; \
	for (i = 0; i<row_name; i++) { \
		imptr = (type_name *)(image_data_pointer + (row_witdh_name) * i); \
		for (j = 0; j<col_name; j++, imptr++) { \
			do_somthing; \
		} \
	} \
}while(0);

#define FUNC_TUPLE_ASSIGN_3(A_SRC, B_SRC, C_SRC)do{\
	A = A_SRC;\
	B = B_SRC;\
	C = C_SRC;\
	}while(0);

#define VAR_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define VAR_MIN(a, b) (((a) < (b)) ? (a) : (b))

/*small micros return pointer to image/matrix according coordination.
* row_xxx_width must defined to tell micros the width of a image.
*/
#define IMAGE_POINTER(img, row, col) (img->im + (row)* row_##img##_width + col)
#define MAT2D_POINTER(mat, row, col) (mat + (row)* row_##mat##_width + col)


/* Part2: Necessary function
*
*/

void throwWarn(const char *s)
{
	static int cnt = 0;

	printf("[WARNING] %s\n", s);
	if (!cnt) {
		printf("Program can proceed with warning, but may yield unexpected results.\n");
		printf("Ctrl+C to end the program if you don't know what's going on.\n");
	}

	cnt++;
}

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

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
#endif