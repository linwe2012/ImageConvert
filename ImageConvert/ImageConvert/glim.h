/* OpenGL Image
* Author: Leon Lin
* PREMATURE Lib for displaying images using openGL..
* So many bugs & TODO... Ughhh.....
*/

#ifndef _GLIM_H_
#define _GLIM_H_
#include "def_utils.h"

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void imshow(Image src);
EXTERNC int glimInit();
/* figure -- create a new window
* fmt:
* w - width
* h - height
* a - anti alising
* c - camera
* s - texture
*/
EXTERNC int figure(const char *title, const char *fmt, ...);
EXTERNC void refreshWindows(int *handles);
EXTERNC int remainingWindows();
EXTERNC void imload_stbi(Image im, const char *path);
#undef EXTERNC
#endif // _GLIM_H_

