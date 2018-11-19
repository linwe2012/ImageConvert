#define _CRT_SECURE_NO_WARNINGS
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
	im->plate = NULL;
	im->name = NULL;
	return im;
}
Image destroyImage(Image im)
{
	free(im->bmpHeader);
	free(im->bmpInfo);
	free(im->im);
	free(im);
	return NULL;
}

/* imcolorCnt -- return how many bytes one pixel takes
*/
int imcolorCnt(Image src){
	if(src->status == rgb || src->status == yuv || src->status == ycbcr || src->status == hsv)
		return 3;
	else
		return 1;
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
		throwWarn("read::Fails to open file.");
		return NULL;
	}
	if (im == NULL) {
		im = newImage();
	}
	fread(im->bmpHeader, sizeof(WORD), 1, fp);
	//sizeof is unreliable cuz system will align elements by 4 bytes
	if (im->bmpHeader->bfType != 0x4D42)
	{
		throwWarn("read::Unknown file type.Will use stbi to load image");
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
	else src->name = (char *)malloc(name_cnt);
	if(s[name_len] == '\\' || s[name_len] == '/' || name_len < 0) ++name_len;
	strcpy(src->name, s + name_len);
}
//This fnction will assume bmp is already padded
//it can write both gray scale & color
//if it is grayscale, it will use color plate and 8bit each pixel
int imwrite(const char  *filePath, Image im) {
	int i;
	if (im == NULL || im->im == NULL) return -1;
	if (_access(filePath, 0) == 0) throwWarn("write::File already exist. Will overwrite it.");
	FILE *fp = fopen(filePath, "wb");
	int color = (im->status == gray || im->status == binary) ? 1 : 3;
	int imagesz = ((color * im->bmpInfo->biWidth + 3) & ~3) * im->bmpInfo->biHeight;
	uint8_t *temp = (uint8_t *)malloc(sizeof(uint8_t) * imagesz);
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
	char *c = fmt;
	visit = (DWORD *)(&fmt + 1);
	if (fmt != NULL) {
		while (*c) {
			if (*c == 'w') {
				iwidth = *visit++;
			}
			else if (*c == 'h') {
				iheight = *visit++;
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
	if (dst == NULL) {
		dst = newImage();
	}
	if (src != dst) resizeImage(dst, row_width * row);
	for (i = 0; i<row; i++) {
		temp = row_width * i;
		sp = (Pixel *)(src->im + temp);
		dp = (Pixel *)(dst->im + temp);
		for (j = 0; j<col; j++, sp++, dp++) {
			R = sp->R;
			G = sp->G;
			B = sp->B;
			
			dp->Y = mat[0][0] * R + mat[0][1] * G + mat[0][2] * B + vec[0];
			dp->U = mat[1][0] * R + mat[1][1] * G + mat[1][2] * B + vec[1];
			dp->V = mat[2][0] * R + mat[2][1] * G + mat[2][2] * B + vec[2];
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
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, row, col, row_width, {
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
	
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, row, col, row_width, {
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
				imptr[0] = (imptr[0] - min) * scaler;
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
	//FUNC_TRAVERSE_PIXEL(image_data_pointer, type_name, row_name, col_name, row_witdh_name, do_somthing)
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, row, col, row_width ,{ 
		if(mmax < imptr->Val) 
			mmax = imptr->Val;
		if(max_Sat < imptr->Sat) 
			max_Sat = imptr->Sat;
		if(min_Sat > imptr->Sat) 
			min_Sat = imptr->Sat;
		sum += imptr->Val / row;
		});
	sum = sum / col;
	image_t log_max = log(mmax + 1);
	image_t temp;
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, row, col, row_width ,{
		temp = (log(imptr->Val + 1)  / log_max);
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
	FUNC_TRAVERSE_PIXEL(src->im, Pixel, (row - 1), (col - 1), row_width ,{
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
	
	FUNC_TRAVERSE_PIXEL(src->im, Pixel, row, col, row_width ,{
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
	image_t inc = 255.0 / row / col;
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, row, col, row_width ,{
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
	FUNC_TRAVERSE_PIXEL(dst->im, Pixel, row, col, row_width ,{
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
			threshold = i;
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
			*dst_ptr = (*src_ptr > threshold) ? 255 : 0;
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
					temp = (*IMAGE_POINTER(op, u, v) < maxBin); 
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
#endif
#if 1
int main()
{
	int idx[] = { 2, -1 };
	
	Image im_lo = imread("./locontrast_tree.bmp", NULL);
	Image hist = rgb2hsv(NULL, im_lo);
	histequal(hist, hist, idx);
	hsv2rgb(hist, hist);
	imwrite("./locontrast_tree_hist.bmp", hist);

	Image im_da = imread("./madrid.jpg", NULL);
	/*
	DEF_IMAGE_ROW_WIDTH(im_da, row, col, row_width);
	Pixel *ptr = NULL;
	printf("row=%d, col=%d, row_width=%d\n", row, col, row_width);
	FUNC_TRAVERSE_PIXEL(im_da->im, Pixel, row, col, row_width, {
		if (ptr > imptr) {
			printf("i=%d,j=%d,%p\t", i ,j, imptr);
		
		}
		ptr = imptr;
		/*
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
		
	});*/
	
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

	printf("-------------------ENDS----------------------\n");
	printf("Press any Key to exit.\n");
	getchar();
	return 0;
}
#endif
