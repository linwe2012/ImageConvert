#ifndef _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS
#include"bmp.h"
#include "glim.h"
#if 1
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <io.h> 
#include <math.h>
#include "config_macros.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "utils/stb_image_write.h"
#include "GLFW/glfw3.h"
#define DIMENSIONS 3

/* function naming method:
* im ## property/do_something ## Appendix
* usually return a property of a image or out/input a image
* 
* do ## Image
*/

Image newImage()
{
	Image im = (Image)malloc(sizeof(ImageBox));
	im->im = NULL;
	im->bmpHeader = (MYBITMAPFILEHEADER *)malloc(sizeof(MYBITMAPFILEHEADER));
	im->bmpInfo = (MYBITMAPFILEINFO *)malloc(sizeof(MYBITMAPFILEINFO));
	im->padded = 0;
	im->status = rgb;
	im->has_opacity = FALSE;
	im->plate = NULL;
	im->name = NULL;
	return im;
}
Image destroyImage(Image im)
{
	printf("Image %s is deleted.\n", im->name);
	free(im->bmpHeader);
	free(im->bmpInfo);
	free(im->im);
	free(im->name);
	if (im->plate) free(im->plate);
	free(im);
	return NULL;
}

/* imcolorCnt -- return how many bytes one pixel takes
*/
int imcolorCnt(Image src){
	//if(src->status == rgb || src->status == yuv || src->status == ycbcr || src->status == hsv)
		//return 3;
	//else
		//return 1;
	if (src->status == gray || src->status == binary)
		return src->has_opacity + 1;
	else
		return src->has_opacity + 3;
}

/* imsize -- calculate image size
* if the image is padded, it will include padding
* if not, then it will return the actual image size without padding
*/
int imsize(Image src){
	int row = src->bmpInfo->biHeight;
	int col = src->bmpInfo->biWidth;
	int row_width = (imcolorCnt(src) * col + 3) & ~3;
	if(src->padded == 1)
		return row * row_width;
	else
		return row * col * imcolorCnt(src);
}

//This function will output bmp with padding
//given the file path and an image, it will read 
//bmp image into image, if image is NULL, it will create a 
//new image.
Image imread(const char *filePath, Image im) {
	int i;
	int biBitCount = 24;
	int imagesz;
	int image_color;
	uint8_t *temp = NULL;
	FILE *fp = fopen(filePath, "rb");
	if (fp == NULL) {
		throwWarn("read::Fails to open file: %s.", filePath);
		return NULL;
	}
	if (im == NULL) {
		im = newImage();
	}
	fread(im->bmpHeader, sizeof(WORD), 1, fp);
	//sizeof is unreliable cuz system will align elements by 4 bytes
	if (im->bmpHeader->bfType != 0x4D42)
	{
		throwNote("read::Unknown file type. Will use stbi to load image");
		fclose(fp);
		imload_stbi(im, filePath);
	}
	else {
		fread((char *)im->bmpHeader + 4, sizeof(DWORD), 3, fp);
		fread(im->bmpInfo, sizeof(MYBITMAPFILEINFO), 1, fp);
		fseek(fp, im->bmpHeader->bfOffBits, SEEK_SET);
		biBitCount = im->bmpInfo->biPlanes_biBitCount >> (sizeof(DWORD) * 4);
		image_color = (int)(biBitCount / 8.0); //compute color

		imagesz = ((image_color * im->bmpInfo->biWidth + 3) & ~3) * im->bmpInfo->biHeight;//(int)(image_color * bmpInfo.biWidth * bmpInfo.biHeight);
		if (im->im != NULL) free(im->im);
		im->im = (image_t *)malloc(sizeof(image_t) * imagesz);
		temp = (uint8_t *)malloc(sizeof(uint8_t) * imagesz);
		fread(temp, sizeof(uint8_t), imagesz, fp);
		for (i = 0; i<imagesz; i++) {
			im->im[i] = temp[i];
		}
		free(temp);
		fclose(fp);
	}
	

	im->padded = 1;
	im->status = (biBitCount == 8) ? gray : rgb;
	
	updateImageName(im, filePath);
	
	return im;
}

/* update Image Name -- update image name according to s
* will omit char till last '\\' or  '/'.
*/ 
void updateImageName(Image src, const char *s){
	int name_len = strlen(s);
	int name_cnt = 0;
	--name_len;
	while(s[name_len] != '\\' && s[name_len] != '/' && name_len >= 0){
		--name_len;
		name_cnt ++;
	}
	if(src->name != NULL) src->name = (char *)realloc(src->name, name_cnt);
	else src->name = (char *)malloc(name_cnt + 1);
	if(s[name_len] == '\\' || s[name_len] == '/' || name_len < 0) ++name_len;
	strcpy(src->name, s + name_len);
}
//This fnction will assume bmp is already padded
//it can write both gray scale & color
//if it is grayscale, it will use color plate and 8bit each pixel
int imwrite(const char  *filePath, Image im) {
	int i;
	if (im == NULL || im->im == NULL) return -1;
	if (_access(filePath, 0) == 0) throwNote("write::File: %s already exist. Will overwrite it.", filePath);
	FILE *fp = fopen(filePath, "wb");
	int color = imcolorCnt(im);
	int row = im->bmpInfo->biHeight;
	int col = im->bmpInfo->biWidth;
	int row_width = (color * col + 3) & ~3;
	int imagesz = row_width * row;
	int success;
	uint8_t *temp = (uint8_t *)malloc(sizeof(uint8_t) * imagesz);
	uint8_t *tmp_end = temp + imagesz;
	if (imagesz > 1 * 1024 * 1024) //over one MB
	{
		fclose(fp);
		int k = 0;
		throwNote("write::The images seems quite big, will use stbi to write. PNG Compression may take a while.");
		FUNC_TRAVERSE_PIXEL(im->im, Pixel, color * sizeof(image_t), (row), col, row_width, FUNC_DO_NOTHING, {
			temp[k++] = CAST_UINT8_T(imptr->B);
			temp[k++] = CAST_UINT8_T(imptr->G);
			temp[k++] = CAST_UINT8_T(imptr->R);
			if(color == 4)
				temp[k++] = CAST_UINT8_T(imptr->A);
		});
		stbi_flip_vertically_on_write(TRUE);
		success = stbi_write_png(filePath, col, row, color, temp, col * color);
		if (!success) {
			throwWarn("Fails to write in files");
		}
		free(temp);
		return 0;
	}
	
	for (i = 0; i<imagesz; i++)
	{
		temp[i] = (uint8_t)(im->im[i]);
	}

	if (im->padded == 0)
		throwWarn("write::The image may not be padded. Output might be inaccurate.");

	fwrite(im->bmpHeader, sizeof(WORD), 1, fp);
	fwrite((char *)im->bmpHeader + 4, sizeof(DWORD), 3, fp);
	fwrite(im->bmpInfo, sizeof(MYBITMAPFILEINFO), 1, fp);
	if ((im->status == gray|| im->status == binary) && im->plate != NULL)
		fwrite(im->plate, sizeof(MYRGBQUAD), 256, fp);
	fwrite(temp, sizeof(uint8_t), imagesz, fp);
	free(temp);
	fclose(fp);
	return 0;
}

/* resizeImage -- resize the image, if dst->im(image data pointer is NULL), malloc one
* if info within 'size' will be preserved since we use realloc
*/
Image resizeImage(Image dst, int size)
{
	if (dst->im == NULL) dst->im = (image_t *)malloc(sizeof(image_t) * size);
	else dst->im = (image_t *)realloc(dst->im, sizeof(image_t) * size);
	return dst;
}

/* duplicateImage -- duplicate source image to destination image.
* if dst is null, then a new image will be created
* if dst is the same as src, it will do nothing and returns the src
*/
Image duplicateImage(Image dst, Image src){
	if(dst == src) return src;
	if(dst == NULL) dst = newImage();
	int imsz = imsize(src);
	resizeImage(dst, imsz);
	copyImageInfo(dst, src);
	memcpy(dst->im, src->im, imsz * sizeof(image_t));
	return dst;
}

/* createPlate -- create color plate for an image, it will always create
* a grayscale plate.
* if image is grayscale, it must has a color plate;
*/
void createPlate(Image dst)
{
	int i;
	MYRGBQUAD *rgbquad = NULL;
	if (dst->plate != NULL) {
		free(dst->plate);
	}
	if (dst->status == gray || dst->status == binary) {
		dst->plate = (MYRGBQUAD *)malloc(sizeof(MYRGBQUAD) * 256);
		rgbquad = dst->plate;
		for (i = 0; i<256; i++)
		{
			rgbquad[i].rgbBlue = i;
			rgbquad[i].rgbGreen = i;
			rgbquad[i].rgbRed = i;
			rgbquad[i].rgbReserved = 0;
			//im->plate[i] = i << 16 + i << 8 + i;//00 01 01 01 little endian
		}
	}
}

/* copyImageInfo -- copy everything except image itself to
* another image.
* it is important that both dst & src exits
* it WON'T create a new Image
*/
void copyImageInfo(Image dst, Image src)
{
	if (dst == NULL || src == NULL) return;
	if (dst == src) return;
	memcpy(dst->bmpHeader, src->bmpHeader, sizeof(MYBITMAPFILEHEADER));
	memcpy(dst->bmpInfo, src->bmpInfo, sizeof(MYBITMAPFILEINFO));
	dst->padded = src->padded;
	dst->status = src->status;
	if (dst->name != NULL) dst->name = (char *)realloc(dst->name, strlen(src->name) + 1);
	else dst->name = (char *)malloc(strlen(src->name) + 1);
	strcpy(dst->name, src->name);
	if (src->plate != NULL) {
		createPlate(dst);
	}
}

void setBmpInfo(Image src, const char* fmt, ...)
{

	WORD ftype = 0x4d42;
	DWORD fresrv = 0x0;
	DWORD foffbits = 0x36;

	DWORD isize = 0x28;
	DWORD iwidth = 0x0;
	DWORD iheight = 0x0;
	DWORD iPlanes_biBitCount = 0x180001;
	DWORD icom = 0x0;
	DWORD iSizeImage = 0x0;;
	LONG iXppm = 0x0;
	LONG iYppm = 0x0;
	DWORD iclrusd = 0x0;
	DWORD iclrvip = 0x0;
	DWORD fSize = iSizeImage + foffbits;
	DWORD *visit;
	const char *c = fmt;
	visit = (DWORD *)(&fmt + 1);
	DWORD tmp;
	if (fmt != NULL) {
		while (*c) {
			if (*c == 'w') {
				iwidth = *visit++;
			}
			else if (*c == 'h') {
				iheight = *visit++;
			}
			else if (*c == 'c') {
				tmp = *visit++;
				iPlanes_biBitCount = iPlanes_biBitCount & 0xFFFF;
				iPlanes_biBitCount = iPlanes_biBitCount | (tmp << 16);
			}
			c++;
		}
	}
	DWORD row_width = (3 * iwidth + 3) & ~3;
	iSizeImage = row_width * iheight;
	fSize = iSizeImage + foffbits;

	src->bmpHeader->bfType = ftype;
	src->bmpHeader->bfSize = fSize;
	src->bmpHeader->bfReserves = fresrv;
	src->bmpHeader->bfOffBits = foffbits;

	src->bmpInfo->biSize = isize;
	src->bmpInfo->biWidth = iwidth;
	src->bmpInfo->biHeight = iheight;
	src->bmpInfo->biPlanes_biBitCount = iPlanes_biBitCount;
	src->bmpInfo->biSizeImage = iSizeImage;
	src->bmpInfo->biCompression = icom;
	src->bmpInfo->biXPelsPerMeter = iXppm;
	src->bmpInfo->biYPelsPerMeter = iYppm;
	src->bmpInfo->biClrUsed = iclrusd;
	src->bmpInfo->biClrImportant = iclrvip;

}
//it dose a matrix maltiplication on a 1*3 vector(RGB or YUV)
//as well as a translation
Image colorSpaceTransform(Image dst, Image src, double mat[DIMENSIONS][DIMENSIONS], double vec[DIMENSIONS])
{
	int i, j, temp;
	image_t R, G, B;
	int row = src->bmpInfo->biHeight;
	int col = src->bmpInfo->biWidth;
	int row_width = (3 * col + 3) & ~3;
	Pixel *sp, *dp, *pp;
	int cnt = 0;
	int color = imcolorCnt(src);
	if (dst == NULL) {
		dst = newImage();
	}
	if (src != dst) resizeImage(dst, row_width * row);

	for (i = 0; i<row; i++) {
		temp = row_width * i;
		sp = (Pixel *)(src->im + temp);
		dp = (Pixel *)(dst->im + temp);
		for (j = 0; j<col; j++) {
			sp = (Pixel*)((image_t*)sp + color);
			dp = (Pixel*)((image_t*)dp + color);

			R = sp->R;
			G = sp->G;
			B = sp->B;
			
			dp->Y = CAST_IMAGE_T(mat[0][0] * R + mat[0][1] * G + mat[0][2] * B + vec[0]);
			dp->U = CAST_IMAGE_T(mat[1][0] * R + mat[1][1] * G + mat[1][2] * B + vec[1]);
			dp->V = CAST_IMAGE_T(mat[2][0] * R + mat[2][1] * G + mat[2][2] * B + vec[2]);
			pp = dp;
			cnt ++;
		}
	}
	copyImageInfo(dst, src);
	return dst;
}


Image rgb2yuv(Image dst, Image src)
{
	double mat[DIMENSIONS][DIMENSIONS] = {
		0.299, 0.587,0.114,
		-0.147,-0.289,0.435,
		0.615,-0.515,-0.1,
	};
	double vec[DIMENSIONS] = {
		0,
		0,
		0,
	};
	
	if(src->status == yuv){
		dst = duplicateImage(dst, src);
		return dst;
	}
	dst = colorSpaceTransform(dst, src, mat, vec);
	dst->status = yuv;
	return dst;
}

Image yuv2rgb(Image dst, Image src)
{
	double mat[DIMENSIONS][DIMENSIONS] = {
		1.0000,    0.0000,    1.1398,
		0.9996,   -0.3954,   -0.5805,
		1.0020,    2.0361,   -0.0005,

	};
	double vec[DIMENSIONS] = {
		0,
		0,
		0,
	};
	if(src->status == rgb){
		dst = duplicateImage(dst, src);
		return dst;
	}
	dst = colorSpaceTransform(dst, src, mat, vec);
	dst->status = rgb;
	return dst;
}

Image rgb2ycbcr(Image dst, Image src)
{
	/*
	double mat[DIMENSIONS][DIMENSIONS] = {
		0.257, 0.564, 0.098,
		-0.148,-0.291, 0.439,
		0.439, -0.368,-0.071,
	};
	double vec[DIMENSIONS] = {
		16,
		128,
		128,
	};*/
	double mat[DIMENSIONS][DIMENSIONS] = {
		0.299, 0.587, 0.114,
		-0.168736,-0.331264, 0.5,
		0.5, -0.418688,-0.081312,
	};
	double vec[DIMENSIONS] = {
		0,
		128,
		128,
	};
	
	if(src->status == ycbcr){
		dst = duplicateImage(dst, src);
		return dst;
	}
	dst = colorSpaceTransform(dst, src, mat, vec);
	dst->status = ycbcr;
	return dst;
}
Image ycbcr2rgb(Image dst, Image src)
{
	double mat[DIMENSIONS][DIMENSIONS] = {
		1,  0.000, 1.402,
		1, -0.344136, -0.714136,
		1,  1.772, 0.000,
	};
	double vec[DIMENSIONS] = {
		-1.402 * 128,
		0.344136 * 128 + 0.714136 * 128 ,
		-1.722 * 128,
	};
	
	if(src->status == rgb){
		dst = duplicateImage(dst, src);
		return dst;
	}
	dst = colorSpaceTransform(dst, src, mat, vec);
	dst->status = rgb;
	return dst;
}

Image rgb2hsv(Image dst, Image src){
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width)
	dst = duplicateImage(dst, src);
	image_t max, min, R, G, B, gap;
	//FUNC_TRAVERSE_PIXEL(image_data_pointer, type_name, step, row_name, col_name, row_witdh_name, do_defore_loop, do_somthing)
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, sizeof(image_t) * imcolorCnt(src), row, col, row_width, FUNC_DO_NOTHING, {
		R = imptr->R;
		G = imptr->G;
		B = imptr->B;
		max = VAR_MAX(R, VAR_MAX(G, B));
		min = VAR_MIN(R, VAR_MIN(G, B));
		gap = max - min;
		imptr->Val = max;
		if(max == 0)
			imptr->Sat = 0;
		else
			imptr->Sat = gap / max;
		
		if(gap == 0)
			imptr->Hue = 0;
		else if(max == R && G >= B)
			imptr->Hue = 60 * (G-B)/gap;
		else if(max == R && G < B)
			imptr->Hue = 360 + 60 * (G-B)/gap;
		else if(max == G)
			imptr->Hue = 120 + 60 * (B-R)/gap;
		else
			imptr->Hue = 240 + 60 * (R-G)/gap;
	})
	dst->status = hsv;
	return dst;
}

Image hsv2rgb(Image dst, Image src){
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width)
	dst = duplicateImage(dst, src);
	image_t H, S, V, A, B, C;
	int Hi;
	image_t f, p, q, t;
	
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, sizeof(image_t) * imcolorCnt(src), row, col, row_width, FUNC_DO_NOTHING,{
		H = imptr->Hue;
		S = imptr->Sat;
		V = imptr->Val;
		Hi = (int)(H / 60) % 6;
		f = H / 60 - Hi;
		p = V * (1 - S);
		q = V * (1 - f * S);
		t = V * (1-(1-f)*S);
		if(Hi == 0)
			FUNC_TUPLE_ASSIGN_3(V, t, p) //its important that NEVER add colon!!!!!!
		else if(Hi == 1)
			FUNC_TUPLE_ASSIGN_3(q, V, p)
		else if(Hi == 2)
			FUNC_TUPLE_ASSIGN_3(p, V, t)
		else if(Hi == 3)
			FUNC_TUPLE_ASSIGN_3(p, q, V)
		else if(Hi == 4)
			FUNC_TUPLE_ASSIGN_3(t, p, V)
		else
			FUNC_TUPLE_ASSIGN_3(V, p, q)
		imptr->R = A;
		imptr->G = B;
		imptr->B = C;
	})
	dst->status = rgb;
	return dst;
}
//rearrange gray scale image to [0,255]
//method 0: set <0 to 0 & >255 to 255
//method 1: linear mapping to [0,255], using method 1 means higher contrast
void rearrangeGrayScale(Image im, int method)
{
	int row = im->bmpInfo->biHeight;
	int col = im->bmpInfo->biWidth;
	int row_witdh = (col + 3) & ~3;
	int i, j;
	double max = 0;
	double min = 999999;
	image_t *imptr = NULL;
	if (method == 1) {
		for (i = 0; i<row; i++) {
			imptr = im->im + row_witdh * i;
			for (j = 0; j<col; j++, imptr++) {
				max = (max > imptr[0]) ? max : imptr[0];
				min = (min < imptr[0]) ? min : imptr[0];
			}
		}
		double scaler = 255.0 / (max - min);
		for (i = 0; i<row; i++) {
			imptr = im->im + row_witdh * i;
			for (j = 0; j<col; j++, imptr++) {
				imptr[0] = CAST_IMAGE_T((imptr[0] - min) * scaler);
			}
		}
	}
	else if (method == 0) {
		double scaler = 255.0 / (max - min);
		for (i = 0; i<row; i++) {
			imptr = im->im + row_witdh * i;
			for (j = 0; j<col; j++, imptr++) {
				if (imptr[0] > 255.0) imptr[0] = 255.0;
				else if (imptr[0] < 0.0) imptr[0] = 0.0;
			}
		}
	}
}

Image grayScale(Image dst, Image src)
{
	Image src_yuv = src;
	int i, j;
	int row = src->bmpInfo->biHeight;
	int col = src->bmpInfo->biWidth;
	int row_src_width = (3 * col + 3) & ~3;
	int row_dst_witdh = (col + 3) & ~3;
	Pixel *sp;
	image_t *dot = NULL;
	if (src->status == gray) {
		return src;
	}
	if (src->status == rgb) {
		src_yuv = newImage();
		rgb2yuv(src_yuv, src);
	}
	if (dst == NULL) {
		dst = newImage();
	}
	if (dst != src) {
		resizeImage(dst, row_dst_witdh * row);
		copyImageInfo(dst, src);
	}
	dot = dst->im;
	for (i = 0; i<row; i++) {
		sp = (Pixel *)(src_yuv->im + row_src_width * i);
		dot = dst->im + row_dst_witdh * i;
		for (j = 0; j<col; j++, sp++, dot++) {
			dot[0] = sp->Y;
		}
	}
	if (dst == src) {
		resizeImage(dst, row_dst_witdh * row);
	}
	DWORD PB = dst->bmpInfo->biPlanes_biBitCount;
	PB = PB & 0xFFFF;
	dst->bmpInfo->biPlanes_biBitCount = PB | (8 << (sizeof(DWORD) * 4));
	dst->status = gray;
	dst->bmpInfo->biSizeImage = row_dst_witdh * row;
	dst->bmpHeader->bfOffBits = 0x36 + 0x400; //color plate
	dst->bmpHeader->bfSize = dst->bmpInfo->biSizeImage + dst->bmpHeader->bfOffBits;
	createPlate(dst);
	if (src_yuv != src) destroyImage(src_yuv);
	return dst;
}
//it will first scale luminace wit a factor scaler
//then add increatment to it;
void luminaceScale(Image im, double scaler, double increatment)
{
	double mat[DIMENSIONS][DIMENSIONS] = {
		scaler,    0.0000,    0.0000,
		0.0000,    1.0000,    0.0000,
		0.0000,    0.0000,    1.0000,
	};
	double vec[DIMENSIONS] = {
		increatment,
		0.0,
		0.0,
	};
	if (im->status != yuv) throwWarn("luminaceScale::Image may be not in YUV color space.");
	colorSpaceTransform(im, im, mat, vec);
}

Image hdr_log(Image dst, Image src)
{
	image_t mmax = -1, max_Sat = -1, min_Sat = 999;
	int cnt = 0;
	double sum = 0;
	// Image tmp = NULL;
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width)
	// tmp = rgb2yuv(tmp, src);
	dst = rgb2hsv(dst, src);
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, sizeof(image_t) * imcolorCnt(dst), row, col, row_width, FUNC_DO_NOTHING,{
		if(mmax < imptr->Val) 
			mmax = imptr->Val;
		if(max_Sat < imptr->Sat) 
			max_Sat = imptr->Sat;
		if(min_Sat > imptr->Sat) 
			min_Sat = imptr->Sat;
		sum += imptr->Val / row;
		});
	sum = sum / col;
	image_t log_max = logf(mmax + 1);
	image_t temp;
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, sizeof(image_t) * imcolorCnt(dst), row, col, row_width, FUNC_DO_NOTHING,{
		temp = (logf(imptr->Val + 1)  / log_max);
		imptr->Val = temp * mmax;
		// imptr->Sat = imptr->Sat * (1 / (temp / 2 + 1) * 0.1 + 1 - 1 / 1.5 * 0.1);
		// temp = imptr->Sat / (max_Sat - min_Sat);
		// imptr->Sat = sqrt(temp) * imptr->Sat;
	});
	dst = hsv2rgb(dst, dst);
	/*if(tmp != src && tmp != dst)
		destroyImage(tmp);*/
	return dst;
}
void denoise_medianFilter(Image src, image_t max, image_t min)
{
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width)
	image_t *ptr;
	int chnl, k, l;
	image_t temp[9], key;
	int step[9] = {
		-row_width - 3, -row_width, -row_width + 3,
		-3,                      0,              3,
		 row_width - 3,  row_width,  row_width + 3,
	};
	FUNC_TRAVERSE_PIXEL(src->im, Pixel, sizeof(image_t) * imcolorCnt(src), (row - 1), (col - 1), row_width, FUNC_DO_NOTHING,{
		if(i==0 || j == 0) continue;
		ptr = (image_t*)(&imptr->Val);
		for(chnl=2;chnl<3;chnl++, ptr++){
			if(*ptr >= max || *ptr <= min){
				for(k=0;k<9;k++){
					l = k;
					key = *(ptr + step[k]);
					while(l && temp[l - 1] > key){
						temp[l] = temp[l - 1];
						l--;
					}
					temp[l] = key;
				}
				*ptr = temp[4];
			}
		}
	});
} 
void smooth_ExpAvg(Image src, image_t beta, image_t threshold)
{
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width)
	image_t *ptr, v_ij[3];
	int chnl;
	for(chnl=0;chnl<3;chnl++)
		v_ij[chnl] = src->im[chnl];
	
	FUNC_TRAVERSE_PIXEL(src->im, Pixel, sizeof(image_t) * imcolorCnt(src), row, col, row_width, FUNC_DO_NOTHING, {
		ptr = (image_t*)imptr;
		for(chnl=0;chnl<3;chnl++, ptr++){
			if(fabs(v_ij[chnl] - *ptr) >= threshold){
				*ptr = (v_ij[chnl] + *ptr) / 2;
			}
			v_ij[chnl] = (1-beta) * v_ij[chnl] + beta * *ptr;
		}
	});
} 

Image histequal(Image dst, Image src, int *chnls)
{
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width);
	
	dst = duplicateImage(dst, src);
	image_t *dp;
	int k, c, buf;
	image_t histogram[3][256] = { 0 };
	image_t inc = CAST_IMAGE_T(255.0 / row / col);
	if (src->status == rgb || src->status == gray) 
	{
		throwWarn("histequal::Image is RGB or Gray, which is not supported.");
	}
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, sizeof(image_t) * imcolorCnt(dst), row, col, row_width, FUNC_DO_NOTHING, {
		dp = (image_t *)imptr;
		for (k = 0; chnls[k] >= 0; k++) {
			buf = (int)(*(dp + chnls[k]));
			if (buf < 0) buf = 0;
			else if (buf > 255) buf = 255;
			histogram[k][buf] += inc;
		}
		//printf("%d, ", (int)(*(dp+chnls[k-1])));
	});
	for(k=0;k<256;k++){
		for(c=0;chnls[c] >= 0; c++){
			if(k) histogram[c][k] = histogram[c][k] + histogram[c][k - 1];
			//printf("h[%d][%d]=%lf, ", c, k, histogram[c][k]);
			// histogram[c][k] = (int)((histogram[c][k] + 0.5));
		}
	}
	int cnt = 0;
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, sizeof(image_t) * imcolorCnt(dst), row, col, row_width, FUNC_DO_NOTHING, {
		dp = (image_t *) imptr; 
		for(k=0;chnls[k] > 0; k++)
			*(dp+chnls[k]) = histogram[k][(int)(*(dp+chnls[k]))];
	});
	return dst;
}
/** otsu method
*/
image_t otsu(Image src)
{
	double histogram[256] = { 0 };
	image_t *imptr, threshold;
	int row = src->bmpInfo->biHeight;
	int col = src->bmpInfo->biWidth;
	int row_witdh = (col + 3) & ~3;
	int imsz = row_witdh * col;
	int i, j;
	double mean = 0, u0_w0 = 0, w0 = 0, g, gMax = -1, temp, div;
	double epsilon = 1e-10;
	//traverse all pixels & collect histogram.
	for (i = 0; i<row; i++) {
		imptr = src->im + row_witdh * i;
		for (j = 0; j<col; j++, imptr++) {
			histogram[(int)(*imptr)] ++;
		}
	}
	//histogram -> probability
	//compute mean
	for (i = 0; i<256; i++) {
		histogram[i] /= 1.0 * imsz;
		mean += i * histogram[i];
	}
	for (i = 0; i<256; i++) {
		u0_w0 += i * histogram[i];
		w0 += histogram[i];
		div = w0 ? w0 : epsilon; //in case divided by 0
		temp = u0_w0 / div - mean; //u0 - mean
		g = temp * temp * w0 / (1 - w0); //current variance
		if (gMax < g) {
			threshold = CAST_IMAGE_T(i);
			gMax = g;
		}
	}
	printf("Threhold = %lf\n", threshold);
	return threshold;
}

/** gray2binary -- compute gray image to binary
* 
*/
Image gray2binary(Image dst, Image src)
{
	int i, j;
	Image grayscale = src;
	image_t *src_ptr, *dst_ptr;
	if (src->status != gray) {
		grayScale(dst, src);
		grayscale = dst;
	}
	int row = grayscale->bmpInfo->biHeight;
	int col = grayscale->bmpInfo->biWidth;
	int row_width = (col + 3) & ~3;
	int imsz = row_width * col;
	if (dst == NULL) {
		dst = newImage();
	}
	if (dst != grayscale) {
		resizeImage(dst, imsz);
		copyImageInfo(dst, grayscale);
	}
	image_t threshold = otsu(grayscale);
	for (i = 0; i<row; i++) {
		src_ptr = grayscale->im + row_width * i;
		dst_ptr = dst->im + row_width * i;
		for (j = 0; j<col; j++, dst_ptr++, src_ptr++) {
			*dst_ptr = CAST_IMAGE_T((*src_ptr > threshold) ? 255 : 0);
		}
	}
	dst->status = binary;
	return dst;
}

/** erode_dilation_filter -- filter image with a convolution kernel.
* kwidth & kheight defines the width & height of a kernel
* will set the result of convolution that smaller than threhold to 0.
* Each pixel of a image will be 0 or maxBin
* note that 0 will be mapped to 0, while maxBin will be mapped to 1
*/
Image erode_dilation_filter(Image dst, Image src, image_t *kernel,int kwidth, int kheight, image_t threshold, image_t maxBin)
{
	int row = src->bmpInfo->biHeight;
	int col = src->bmpInfo->biWidth;
	int row_op_width = (col + 3) & ~3;
	int row_dst_width = row_op_width;
	int row_kernel_width = kwidth;
	int imsz = row_op_width * col;
	int startx =(int)((kwidth) / 2);
	int starty = (int)((kheight) / 2);
	int endx = row - startx;
	int endy = col - starty;
	int i, j, u, v, x, y;
	Image op = src;
	image_t res, temp;
	if (src->status != binary)
		throwWarn("erode_dilation_filter::the source image is not Binary Bitmap");
	if (dst == NULL) {
		dst = newImage();
	}
	if (dst == src) {
		op = newImage();
		resizeImage(op, imsz);
		memcpy(op->im, src->im, imsz * sizeof(image_t));
	}
	if (dst != src) {
		resizeImage(dst, imsz);
		copyImageInfo(dst, src);
		memcpy(dst->im, src->im, imsz * sizeof(image_t));
	}
	for (i = startx; i < endx; i++) {
		for (j = starty; j < endy; j++) {
			res = 0;
			x = 0;
			// convolution
			for (u = i - startx; u <= i + startx; u++) {
				y = 0;
				for (v = j - starty; v <= j + starty; v++) {
					//determine if this pixel is 0 or 1
					temp = CAST_IMAGE_T(*IMAGE_POINTER(op, u, v) < maxBin);
					res += *MAT2D_POINTER(kernel, x, y) * temp;
					y++;
				}
				x++;
			}
			// eleminate all pixels smaller than threshold.
			*IMAGE_POINTER(dst, i, j) = (res >= threshold) ? 0 : maxBin;
		}
	}
	if (op != src) {
		destroyImage(op);
	}
	return dst;
}
/** dilation -- set threhold to one, use erode_dilation_filter to solve.
*/
Image dilation(Image dst, Image src, image_t *kernel, int kwidth, int kheight)
{
	return erode_dilation_filter(dst, src, kernel, kwidth, kheight, 1, 255);
}
/** erode -- set threhold to L1 norm of kernel, use erode_dilation_filter to solve.
*/
Image erode(Image dst, Image src, image_t *kernel, int kwidth, int kheight)
{
	image_t threshold = 0;
	int row_kernel_width = kwidth;
	int i, j;
	for (i = 0; i < kwidth; i++) {
		for (j = 0; j < kheight; j++) {
			threshold += *MAT2D_POINTER(kernel, i, j);
		}
	}
	return erode_dilation_filter(dst, src, kernel, kwidth, kheight, threshold, 255);
}
/** opening -- first erode then dilate.
*/
Image opening(Image dst, Image src, image_t *kernel, int kwidth, int kheight)
{
	dst = erode(dst, src, kernel, kwidth, kheight);
	return dilation(dst, dst, kernel, kwidth, kheight);
}
/** closing -- first dilate then erode.
*/
Image closing(Image dst, Image src, image_t *kernel, int kwidth, int kheight)
{
	dst = dilation(dst, src, kernel, kwidth, kheight);
	return erode(dst, dst, kernel, kwidth, kheight);
}

/* newSTConfig
* fmt: specifies which configures will override default
* * * fmt can be NULL, which everything will be default
* ...: 
**** flags *****
* i: Interpolation_Func
* * - 0: BinlinerInterpolation
* l,r,t,b: left[sinistral] right[dextral] top[up] bottom[down], boundries -- must be doubles!!!!
* o: opacity -- int
* R,G,B,A: empty pixelse -- double
*/
STConfig newSTConfig(const char *fmt, ...)
{
	STConfig stc = (STConfig)malloc(sizeof(struct tagSTConfig));
	const char* wander = fmt;
	char *visit = (char *)(&fmt + 1);
	double *fptr = NULL;
	image_t *imptr = NULL;

	stc->itp = BinlinerInterpolation;
	stc->has_opacity = 1;
	stc->default_alpha = CAST_IMAGE_T(255);

	stc->bounds[0].x = -1;
	stc->bounds[0].y = -1;
	stc->bounds[0].z = -1;
	stc->bounds[1].x = -1;
	stc->bounds[1].y = -1;
	stc->bounds[1].z = -1;


	memset(&stc->empty, 0, sizeof(Pixel)); //pixel by default is all 0

	if (fmt != NULL) {
		while (*wander) {
			if (*wander == 'i') {

			}
			else if(*wander == 'l' || *wander == 'r' || *wander == 't' || *wander == 'b')
			{
				switch (*wander) {
				case 'l': fptr = &stc->bounds[0].x; break;
				case 'r': fptr = &stc->bounds[1].x; break;
				case 't': fptr = &stc->bounds[0].y; break;
				case 'b': fptr = &stc->bounds[1].y; break;
				default: //do nothing -- impossible
					break;
				}
				*fptr = *(double *)visit;
				visit += sizeof(double);
			}
			else if (*wander == 'o')
			{
				stc->has_opacity = (*(int *)visit) ? 1 : 0;
				visit += sizeof(int);
			}
			else if (*wander == 'R' || *wander == 'G' || *wander == 'B' || *wander == 'A') 
			{
				switch (*wander) {
				case 'R':imptr = &stc->empty.R; break;
				case 'G':imptr = &stc->empty.G; break;
				case 'B':imptr = &stc->empty.B; break;
				case 'A':imptr = &stc->empty.A; break;
				default: //do nothing -- impossible
					break;
				}
				*imptr = CAST_IMAGE_T(*(double *)visit);
				visit += sizeof(double);
			}
			wander++;
		}
	}
	return stc;
}
STConfig destroySTConfig(STConfig stc) {
	free(stc);
	return NULL;
}

void BinlinerInterpolation(Image src, Pixel *res, double x, double y, int color)
{
	DEF_IMAGE_ROW_WIDTH(src, src_row, src_col, row_src_width);
	int x0 = CAST_INT(x);
	int y0 = CAST_INT(y);
	double dx = x - x0;
	double dy = y - y0;
	image_t *ptr = (src->im + row_src_width * y0 + x0 * color);//28d58
	image_t *src_ptr;
	image_t *res_ptr = (image_t *)res;
	int i;
	memset(res, 0, sizeof(Pixel)); //set result to 0;
	/*
	  (x0,y0)----------(x1, y0)------>x
		|        |
		|        dy       
		|        |
		|--dx--(x,y)      
		|
	  (x0,y1)----------(x1, y1)
	    |
		|
	   y\/
	*/
	image_t *test = ptr;
	for (i = 0; i < color; i++) {
		src_ptr = ptr;
		*res_ptr += CAST_IMAGE_T(*src_ptr * (1 - dx)*(1 - dy)); //top left

		src_ptr = ptr + color;
		*res_ptr += CAST_IMAGE_T(*src_ptr * dx * (1 - dy)); //top right

		src_ptr = ptr + row_src_width;
		*res_ptr += CAST_IMAGE_T(*src_ptr * (1 - dx) * dy); //bottom left

		src_ptr += color;
		*res_ptr += CAST_IMAGE_T(*src_ptr * dx * dy); //bottom right

		
		//shift to next channel
		ptr++;
		res_ptr++;
	}
}

/* spatialTransform transform image according to inverted matrix
* dst, src must already allocated with space
* invmat: inverted matrix, a matrix transform dst coordinates into src coordinates
* stc: specifies Interpolation method, default pixel value, and transform region
*/
Image spatialTransform(Image dst, Image src, double invmat[SPATIAL_DIMS][SPATIAL_DIMS], STConfig stc)//Pixel empty, Interpolation_Func itp, int top, int right, int bottom, int left)
{
	int src_color = imcolorCnt(src);
	int dst_color = imcolorCnt(dst);
	DEF_IMAGE_ROW_WIDTH(dst, dst_row, dst_col, row_dst_width);
	DEF_IMAGE_ROW_WIDTH(src, src_height, src_width, row_src_width);
	double src_x, src_y;
	image_t *collector, *sink;
	Pixel collect;
	int chnl;
	int top    = CAST_INT((stc->bounds[0].y<0)? 1          :stc->bounds[0].y);
	int right  = CAST_INT((stc->bounds[1].x<0)? src_width  :stc->bounds[1].x);
	int bottom = CAST_INT((stc->bounds[1].y<0)? src_height :stc->bounds[1].y);
	int left   = CAST_INT((stc->bounds[0].x<0)? 1          :stc->bounds[0].x);
	int flag_has_alpha = 0;
	Interpolation_Func itp = stc->itp;

	image_t *sink_end = dst->im + dst_row * row_dst_width;
	// printf("dst row=%d, col=%d, color=%d, row_wi=%d\n", dst_row, dst_col, dst_color, row_src_width);
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, sizeof(image_t) * dst_color, dst_row - 1, dst_col, row_dst_width, FUNC_DO_NOTHING, {
		src_x = invmat[0][0] * j + invmat[0][1] * i + invmat[0][2];
		src_y = invmat[1][0] * j + invmat[1][1] * i + invmat[1][2];

		flag_has_alpha = (src_color == 4);

		//out of image boundry
		//sample from user defined empty Pixel
		if (src_x >= src_width - 1 || src_x <= 0 || src_y >= src_height - 1 || src_y <= 0) {
			flag_has_alpha = 1;
			collector = (image_t *)&stc->empty;
		}
		//out of spatial transform boundry
		//directly sample from image at exact point
		else if (src_x > right - 1 || src_x < left - 1 || src_y > bottom - 1 || src_y < top - 1) 
			collector = src->im + i * row_src_width + j * src_color;
		//collector happens to sit on the image boundry, may cause read vilation for Interpolation function
		//sample from edge without any interpolation
		else if(src_x == 0 || src_y == 0 || src_x == src_width - 1 || src_y == src_height){
			collector = src->im + CAST_INT(src_y) * row_src_width + CAST_INT(src_x) * src_color;
		}
		else {
			itp(src, &collect, src_x, src_y, src_color);
			collector = (image_t *)&collect;
		}

		//Pour the color collected from source into sink (aka. dst)
		sink = (image_t *)(imptr);
		for (chnl = 0; chnl < dst_color; chnl++) 
			if (chnl == 3 && !flag_has_alpha) //srouce has no alpha channel but sink requires a alpha channel
				*sink++ = stc->default_alpha;
			else
				*sink++ = *collector++;
		if (sink_end - sink < 0)
		{
			printf("overflow by %d bytes i=%d,j=%d, \n", i, j, sink- sink_end);
			getchar();
		}
	});
	return dst;
}
#endif

/* imtranslate -- translate a image and return the result.
* x means move right x pixels, while y moves image downwards.
*/
Image imtranslate2d(Image src, Vector3 direction, STConfig stc)
{
	Image dst = newImage();
	int width = src->bmpInfo->biWidth + CAST_INT(fabs(direction.x));
	int height = src->bmpInfo->biHeight + CAST_INT(fabs(direction.y));
	dst->status = src->status;
	dst->has_opacity = (stc->has_opacity) ? 1 : 0;
	int color = imcolorCnt(dst);
	int ww = (color * width + 3) & ~3;

	setBmpInfo(dst, "whc", width, height, color * 8);
	resizeImage(dst, ww * height + 1);

	double invmat[SPATIAL_DIMS][SPATIAL_DIMS] = {
		1, 0, -direction.x,
		0, 1, -direction.y,
		0, 0, 1,
	};

	return spatialTransform(dst, src, invmat, stc);
}

/* imrotate2d -- rotate image along an axis by theta anti-clockwise
* theta: the rotation angle
* axis: rotation axis, note that only x, y coordinates are read, rests are discarded.
* stc: spatial transform configuration
* output: a new image after roatation.
*/
Image imrotate2d(Image src, double theta, Vector3 *axis, STConfig stc)
{
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width);
	double ct = cos(theta);
	double st = sin(theta);
	int width  = CAST_INT(fabs(row * st) + fabs(col * ct) + 1);
	int height = CAST_INT(fabs(row * ct) + fabs(col * st) + 1);
	Image dst = newImage();
	Vector3 tmpaxis;
	double x_hat, y_hat;
	dst->has_opacity = (stc->has_opacity) ? 1:0;
	int color_channel = imcolorCnt(dst);
	int ww = (color_channel * width + 3) & ~3;
	int flag_stcDestroy = 0;

	//axis by default is image center
	if (axis == NULL) {
		tmpaxis.x = col / 2.0;
		tmpaxis.y = row / 2.0;
		axis = &tmpaxis;
	}
	if (stc == NULL) {
		flag_stcDestroy;
		stc = newSTConfig(NULL);
	}
	//include translation into consideration
	x_hat = axis->x + (width - col)  / 2.0;
	y_hat = axis->y + (height - row) / 2.0;

	double invmat[SPATIAL_DIMS][SPATIAL_DIMS] = {
		 ct,  st, -x_hat * ct - y_hat * st + axis->x,
		-st,  ct,  x_hat * st - y_hat * ct + axis->y,
		  0,   0,  1,
	};
	resizeImage(dst, ww * height + 1);
	setBmpInfo(dst, "whc", width, height, color_channel * 8);
	dst->padded = 1;
	
	Pixel empty = { 0 , 0, 0, 0 };
	spatialTransform(dst, src, invmat, stc);

	if (flag_stcDestroy) destroySTConfig(stc);
	return dst;
}

/* imscale2d -- scale the image as specified by [vertor3]scaler
* scaler: the scaler, in 2d cases, only x, y is concerned.
* stc: spatial transform configuration - ignores has_opacity.
* output: a new image after roatation.
*/
Image imscale2d(Image src, Vector3 scaler, STConfig stc)
{
	Image dst = newImage();
	int width = CAST_INT(src->bmpInfo->biWidth * scaler.x);
	int height = CAST_INT(src->bmpInfo->biHeight * scaler.y);
	int ww = (imcolorCnt(src) * width + 3) & ~3;

	setBmpInfo(dst, "wh", width, height);
	resizeImage(dst, height * ww + 1);

	double invmat[SPATIAL_DIMS][SPATIAL_DIMS] = {
		1.0 / scaler.x, 0, 0,
		0, 1.0 / scaler.y, 0,
		0, 0, 1,
	};

	return spatialTransform(dst, src, invmat, stc);
}

/* imflip2d -- flip the image as specified by mirror line
* mirror: mirror line, 
* * - if mirror.x > 0, then flip around x
* * - if mirror.y > 0, flip around y
* * - if both x,y > 0, flip around x and y
* stc: spatial transform configuration - ignores has_opacity.
* output: a new image after roatation.
*/
Image imflip2d(Image src, Vector3 mirror, STConfig stc)
{
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width);
	Image dst = newImage();

	copyImageInfo(dst, src);
	resizeImage(dst, row_width * row + 1);

	double x = 1.0, y = 1.0;
	double w = 0.0, h = 0.0;
	if (mirror.x > 0) {
		x = -1.0;
		w = col-1;
	}
	if (mirror.y > 0) {
		y = -1.0;
		h = row-1;
	}
	double invmat[SPATIAL_DIMS][SPATIAL_DIMS] = {
		x, 0, w,
		0, y, h,
		0, 0, 1,
	};
	return spatialTransform(dst, src, invmat, stc);
}

Image imshear2d(Image src, Vector3 factor, STConfig stc)
{
	Image dst = newImage();
	DEF_IMAGE_ROW_WIDTH(src, row, col, row_width);
	int width  = col + CAST_INT(fabs(factor.x) * row);
	int height = row + CAST_INT(fabs(factor.y) * col);
	dst->has_opacity = (stc->has_opacity) ? 1 : 0;
	dst->status = src->status;
	int color = imcolorCnt(dst);
	int ww = (color * width + 3) & ~3;
	resizeImage(dst, ww * height + 1);
	setBmpInfo(dst, "whc", width, height, color * 8);
	double denomitor = 1 - factor.x * factor.y;
	if (denomitor == 0) {
		denomitor = 0.000001;
	}
	double invmat[SPATIAL_DIMS][SPATIAL_DIMS] = {
		1.0 / denomitor, -factor.x / denomitor, 0,
		-factor.y / denomitor,1.0/denomitor, 0,
		0, 0, 1,
	};
	return spatialTransform(dst, src, invmat, stc);
}

void throwWarn(const char *s, ...)
{
	static int cnt = 0;

	GLIM_LOG_OUTPUT("[WARNING] ");

	va_list args;
	va_start(args, s);
	GLIM_LOG_OUTPUT_V(s, args);
	va_end(args);

	GLIM_LOG_OUTPUT("\n");


	if (!cnt) {
		GLIM_LOG_OUTPUT("Program can proceed with warning, but may yield unexpected results.\n");
		GLIM_LOG_OUTPUT("Ctrl+C to end the program if you don't know what's going on.\n");
	}
	cnt++;
}

void throwNote(const char *s, ...)
{
	GLIM_LOG_OUTPUT("[Note] ");

	va_list args;
	va_start(args, s);
	GLIM_LOG_OUTPUT_V(s, args);
	va_end(args);

	GLIM_LOG_OUTPUT("\n");
}

#if 0
int main()
{
	int idx[] = { 2, -1 };

	

	
	Image im = imread("./madrid_hdr.bmp", NULL);
	
	STConfig stc = newSTConfig(NULL);

	Image im_ro = imrotate2d(im, 0.5, NULL, stc);
	throwNote("Rotation Complete. Ready to write to file.");
	imwrite("./madrid_rotate.png", im_ro);
	im_ro = destroyImage(im_ro);
	throwNote("Image After rotation saved at madrid_rotate.png");
	
	Vector3 factor = {0.1, 0.05, 0};
	Image im_shear = imshear2d(im, factor, stc);
	imwrite("./madrid_shear.png", im_shear);
	im_shear = destroyImage(im_shear);
	throwNote("Image After shear saved at madrid_shear.png");
	
	Image im_mirror = imflip2d(im, factor, stc);
	imwrite("./madrid_mirror.png", im_mirror);
	im_mirror = destroyImage(im_mirror);
	throwNote("Image After flipped saved at madrid_mirror.png");
	
	Vector3 scaler = { 0.7, 1.01, 0 };
	Image im_large = imscale2d(im, scaler, stc);
	imwrite("./madrid_scale_large.png", im_large);
	im_large = destroyImage(im_large);
	throwNote("Image After scaled saved at madrid_scale_large.png");
	
	Vector3 direction = { 300, 100, 0 };
	Image im_trans = imtranslate2d(im, direction, stc);
	imwrite("./madrid_trans.png", im_trans);
	im_trans = destroyImage(im_trans);
	throwNote("Image After translation saved at madrid_trans.png");
	
	/*
	Image hist = rgb2hsv(NULL, im_lo);
	histequal(hist, hist, idx);
	hsv2rgb(hist, hist);
	imwrite("./locontrast_tree_hist.bmp", hist);

	Image im_da = imread("./madrid.jpg", NULL);
	Image hdr = hdr_log(im_da, im_da);
	imwrite("./madrid_hdr.bmp", hdr);

	printf("Showing image is still an experimental feature.\nDue to my limited kownlegde of OpenGL, it consumes extravagant resources.\nWill you still want to display image? Y/n:");
	char c = getchar();
	int showimg = 1;
	if (c == 'n' || c == 'N' ) {
		showimg = 0;
		printf("Image display is suppressed.\n");
	}
	fflush(stdin);
	if (showimg) {
		glimInit();
		figure("Low Contrast Trees after histogram equalization", "wsc", 1000, SHADER_TEXTURE|SHADER_CAMERA, CAMERA_HORIZONTAL);
		imshow(hist);
		figure("HDR image of Sunset of Madrid", "whcs", 1400, 800 , (0x1 << 1), 0x3);
		imshow(hdr);
		while (remainingWindows()) {
			refreshWindows(NULL);
		}
	}
	*/
	printf("-------------------ENDS----------------------\n");
	printf("Press any Key to exit.\n");
	getchar();
	return 0;
}
#endif

#if 0
Global_Images.imgs = (Image *)malloc(sizeof(Image) * 20);
Global_Images.cnt = 0;
Global_Images.max = 20;
typedef struct tagImageUnion ImageUnion;
struct tagImageUnion {
	Image *imgs;
	int cnt;
	int max;
};
ImageUnion Global_Images;
void fileDealer(GLFWwindow* window, int count, const char** paths)
{
	int i;
	for (i = 0; i < count; i++) {
		Global_Images.imgs[Global_Images.cnt++] = imread(paths[i], NULL);
	}
	STConfig stc = newSTConfig(NULL);
	Image im_ro = imrotate2d(Global_Images.imgs[0], 0.5, NULL, stc);
	imshow(im_ro);
	destroyImage(im_ro);
	printf("HI, Drop Called");
}
// [in] int main()
DEF_IMAGE_ROW_WIDTH(im_da, row, col, row_width);
Pixel *ptr = NULL;
printf("row=%d, col=%d, row_width=%d\n", row, col, row_width);
FUNC_TRAVERSE_PIXEL(im_da->im, Pixel, row, col, row_width, {
	if (ptr > imptr) {
		printf("i=%d,j=%d,%p\t", i ,j, imptr);

	}
ptr = imptr;

ptr = (Pixel *)(im_dda->im + ((image_t *)imptr - im_da->im));
if (fabsf(imptr->R - ptr->R) > 3) {
printf("R:i=%d,j=%d,m=%f,pos=%p,opos=%p\n", i, j, imptr->R - ptr->R, imptr, ptr);
}
else if (fabsf(imptr->G - ptr->G) > 3) {
printf("G:i=%d,j=%d,m=%f,pos=%p,opos=%p\n", i, j, imptr->G - ptr->G, imptr, ptr);
}
else if (fabsf(imptr->B - ptr->B) > 3) {
printf("B:i=%d,j=%d,m=%f,pos=%p,opos=%p\n", i, j, imptr->B - ptr->B, imptr, ptr);
}

});
#endif