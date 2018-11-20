# ImageConvert
![compiler](https://img.shields.io/badge/Compiler-Visual%20Studio%202017-blue.svg)

![progress](https://img.shields.io/badge/coverage-15%25-yellowgreen.svg?maxAge=2592000)

ZJU Image Info Proc Course Archieve. All in one.


Including [APIs](https://github.com/linwe2012/ImageConvert/blob/master/ImageConvert/ImageConvert/bmp.h):
```c++
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

```

# Acknowlegdement

For the original `shader.h` and `camera.h`: [LearnOpenGL](https://learnopengl.com/)

This project depends on may libs, including

- [GLFW](https://www.glfw.org/), which is used for creating windows, receiving input and events.

- [GLAD](https://github.com/Dav1dde/glad) manages all the manages all that cumbersome work. [Release Page](https://glad.dav1d.de/).

- [GLM](https://github.com/g-truc/glm) handles tricky math

- [stbi](https://github.com/nothings/stb) of Sean Barrett handles image Input except `.bmp`
 
