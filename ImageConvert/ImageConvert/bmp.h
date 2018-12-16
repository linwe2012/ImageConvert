#ifndef _BMP_H_
#define _BMP_H_
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "def_utils.h"

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC Image imread(const char *filePath, Image im);
EXTERNC int imwrite(const char  *filePath, Image im);
EXTERNC void updateImageName(Image src, const char *s);

/******************************
* color space transformations
*/
EXTERNC Image rgb2yuv(Image dst, Image src);
EXTERNC Image yuv2rgb(Image dst, Image src);
EXTERNC Image rgb2ycbcr(Image dst, Image src);
EXTERNC Image ycbcr2rgb(Image dst, Image src);
EXTERNC Image rgb2hsv(Image dst, Image src);
EXTERNC Image hsv2rgb(Image dst, Image src);

//functions operates on color transformed images
EXTERNC Image grayScale(Image dst, Image src);
EXTERNC void rearrangeGrayScale(Image im, int method);
EXTERNC void luminaceScale(Image im, double scaler, double increatment);


/******************************
* Image Enchancement
*/
//binary images
EXTERNC Image gray2binary(Image dst, Image src);
EXTERNC Image dilation(Image dst, Image src, image_t *kernel, int kwidth, int kheight);
EXTERNC Image erode(Image dst, Image src, image_t *kernel, int kwidth, int kheight);
EXTERNC Image opening(Image dst, Image src, image_t *kernel, int kwidth, int kheight);
EXTERNC Image closing(Image dst, Image src, image_t *kernel, int kwidth, int kheight);

//contrast optimize
EXTERNC Image hdr_log(Image dst, Image src);
EXTERNC Image histequal(Image dst, Image src, int *chnls);

//image denoise & smooth
EXTERNC void denoise_medianFilter(Image src, image_t max, image_t min);
EXTERNC void smooth_ExpAvg(Image src, image_t beta, image_t threshold);

//image filter using convolution
EXTERNC Image imconv2(Image dst, Image src, Conv2ConfigBox *ccb);
EXTERNC image_t* fspecial(const char *name, ...);
EXTERNC Image imfilter(Image src, image_t *kernel, int *filtersz, Vector3 bounds[]);
EXTERNC void imfuse(Image lhs, const Image rhs, image_t alpha, image_t beta, image_t delta, int *layers);

/******************************
* spatial transformations
*/
//auxilaries for spacial transform
EXTERNC void BinlinerInterpolation(Image src, Pixel *res, double x, double y, int color);
EXTERNC STConfig newSTConfig(const char *fmt, ...);

//implement spatial transform
EXTERNC Image imtranslate2d(Image src, Vector3 direction, STConfig stc);
EXTERNC Image imrotate2d(Image src, double theta, Vector3 *axis, STConfig stc);
EXTERNC Image imscale2d(Image src, Vector3 scaler, STConfig stc);
EXTERNC Image imflip2d(Image src, Vector3 mirror, STConfig stc);
EXTERNC Image imshear2d(Image src, Vector3 factor, STConfig stc);

#undef EXTERNC
#endif
