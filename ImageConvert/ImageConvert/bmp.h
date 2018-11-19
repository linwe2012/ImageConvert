#ifndef _BMP_H_
#define _BMP_H_
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "def_utils.h"

Image imread(const char *filePath, Image im);
int imwrite(const char  *filePath, Image im);
void updateImageName(Image src, const char *s);

/******************************
* color space transformations
*/
Image rgb2yuv(Image dst, Image src);
Image yuv2rgb(Image dst, Image src);
Image rgb2ycbcr(Image dst, Image src);
Image ycbcr2rgb(Image dst, Image src);
Image rgb2hsv(Image dst, Image src);
Image hsv2rgb(Image dst, Image src);

//functions operates on color transformed images
Image grayScale(Image dst, Image src);
void rearrangeGrayScale(Image im, int method);
void luminaceScale(Image im, double scaler, double increatment);


/******************************
* Image Enchancement
*/
//binary images
Image gray2binary(Image dst, Image src);
Image dilation(Image dst, Image src, image_t *kernel, int kwidth, int kheight);
Image erode(Image dst, Image src, image_t *kernel, int kwidth, int kheight);
Image opening(Image dst, Image src, image_t *kernel, int kwidth, int kheight);
Image closing(Image dst, Image src, image_t *kernel, int kwidth, int kheight);

//contrast optimize
Image hdr_log(Image dst, Image src);
Image histequal(Image dst, Image src, int *chnls);

//image denoise & smooth
void denoise_medianFilter(Image src, image_t max, image_t min);
void smooth_ExpAvg(Image src, image_t beta, image_t threshold);


#endif
