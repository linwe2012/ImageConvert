/* OpenGL Image
* Author: Leon Lin
* PREMATURE Lib for displaying images using openGL..
* So many bugs & TODO... Ughhh.....
*/

#ifndef _GLIM_H_
#define _GLIM_H_
#ifndef _CRT_SECURE_NO_WARNINGS
#define  _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS

#include "def_utils.h"
#include <glad\glad.h>
#include <GLFW\glfw3.h>
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
EXTERNC GLFWwindow *figure(const char *title, const char *fmt, ...);
EXTERNC void registerDropFile(GLFWwindow *win, GLFWdropfun dropfun);
EXTERNC void refreshWindows(int *handles);
EXTERNC int remainingWindows();
EXTERNC void imload_stbi(Image im, const char *path);
// EXTERNC void addTextByPointer(GLFWwindow *win, Vector3 pos, Vector3 color, float scale, const char *indicator);
// EXTERNC void setImageProcessWindow(GLFWwindow *glfwwin, GLFWdropfun dropfun);
#undef EXTERNC
#endif // _GLIM_H_

