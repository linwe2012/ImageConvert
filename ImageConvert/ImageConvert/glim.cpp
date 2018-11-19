#define  _CRT_SECURE_NO_WARNINGS
#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include "shader.h"
#include "glim.h"
#include "lmath.h"
#include "Window.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

std::map<int, Window*> windows;
std::vector<Window*> windows_recyle;
Window* currentWindow = NULL;
static int WindowID = 0;
int glimInit()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	srand(static_cast<unsigned int>(time(NULL)));
	int major, minor, revision;
	glfwGetVersion(&major, &minor, &revision);
	printf("Running against GLFW %i.%i.%i\n", major, minor, revision);
	return 0;
}
/* figure -- create a new window
* fmt:
* w - width
* h - height
* a - anti alising
* c - camera
* s - texture
*/
int figure(const char *title, const char *fmt, ...)
{
	const char *default_title = "Open GL";
	int *looking = (int *)&fmt;
	int width = SCR_WIDTH;
	int height = SCR_HEIGHT;
	int anti_alising = TRUE;
	int camera = CAMERA_DISABLE;
	int shader = SHADER_TEXTURE;
	if (title == NULL) title = default_title;
	if (fmt != NULL) {
		while (*fmt) {
			if (*fmt == 'w') {
				width = *(++looking);
			}
			else if (*fmt == 'h') {
				height = *(++looking);
			}
			else if (*fmt == 'a') {
				anti_alising = *(++looking);
			}
			else if (*fmt == 'c') {
				camera = *(++looking);
			}
			else if (*fmt == 's') {
				shader = *(++looking);
			}
			fmt++;
		}
	}
	Window *window = new Window(title, width, height, anti_alising, camera);
	window->initShader(shader);
	windows[WindowID] = window;
	currentWindow = window;
	return WindowID++;
}

void refreshWindows(int *handles)
{
	Window *win;
	if (handles == NULL) {
		std::map<int, Window*>::iterator itr;
		for (itr = windows.begin(); itr != windows.end();) {
			itr->second->refresh();
			if (itr->second->window == NULL) {
				win = itr->second;
				windows.erase(itr++);
				delete win;
			}
			else
				itr++;
		}
	}
	else {
		for (int *ptr = handles; *ptr > 0; ptr++) {
			win = windows[*ptr];
			win->refresh();
			if (win->window == NULL) {
				windows.erase(*ptr);
				delete win;
			}
			
		}
	}
}

int remainingWindows()
{
	return windows.size();
}
void imload_stbi(Image im, const char *path)
{
	int col;
	int row;
	int color;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(path, &col, &row, &color, 3);
	int row_width = (3 * col + 3) & ~3;
	resizeImage(im, row * row_width);
	setBmpInfo(im, "wh", col, row);
	unsigned char *ptr = data;
	FUNC_TRAVERSE_PIXEL(im->im, Pixel, row, col, row_width, {
		imptr->B = *ptr++;
		imptr->G = *ptr++;
		imptr->R = *ptr++;
		if (imptr->R < 0 || imptr->R > 255) {
			printf("R:%f,\t", imptr->R);
		}
		if (imptr->G < 0 || imptr->G > 255) {
			printf("G:%f,\t", imptr->G);
		}
		if (imptr->B < 0 || imptr->B > 255) {
			printf("B:%f,\t", imptr->B);
		}

	});
	free(data);
}
void imshow(Image src)
{
	int row = src->bmpInfo->biHeight;
	int col = src->bmpInfo->biWidth;
	int row_width = (3 * col + 3) & ~3;
	int osz[] = { row, col };
	int size[2] = { 1, 1 };
	int gap[2] = { 0 };
	int k;
	for (k = 0; k < 2; k++) {
		while (size[k] < osz[k]) {
			size[k] *= 2;
		}
		gap[k] = (size[k] - osz[k]) / 2;
	}
	unsigned char *data = new unsigned char[size[0] * size[1] * 3 + 1];
	unsigned char *ptr = data;
	do {
		int i, j;
		Pixel *imptr = NULL;
		for (i=0; i < gap[0]; i++) {
			for (j = 0; j < size[1]; j++) {
				*ptr++ = static_cast<unsigned char>(51);
				*ptr++ = static_cast<unsigned char>(0.3 * 255);
				*ptr++ = static_cast<unsigned char>(0.3 * 255);
			}
		}
		for (; i < row + gap[0]; i++) {
			imptr = (Pixel *)(src->im + (row_width)* (i - gap[0]));
			for (j = 0; j < gap[1]; j++) {
				*ptr++ = static_cast<unsigned char>(51);
				*ptr++ = static_cast<unsigned char>(0.3 * 255);
				*ptr++ = static_cast<unsigned char>(0.3 * 255);
			}
			for (; j<col+gap[1]; j++, imptr++) {
				*ptr++ = static_cast<unsigned char>(imptr->B);
				*ptr++ = static_cast<unsigned char>(imptr->G);
				*ptr++ = static_cast<unsigned char>(imptr->R);
			}
			for (; j < size[1]; j++) {
				*ptr++ = static_cast<unsigned char>(51);
				*ptr++ = static_cast<unsigned char>(0.3 * 255);
				*ptr++ = static_cast<unsigned char>(0.3 * 255);
			} 
		} 
		for (; i < size[0]; i++) {
			for (j = 0; j < size[1]; j++) {
				*ptr++ = static_cast<unsigned char>(51);
				*ptr++ = static_cast<unsigned char>(0.3 * 255);
				*ptr++ = static_cast<unsigned char>(0.3 * 255);
			}
		}
	} while (0);

	currentWindow->diplayImage(data, size[1], size[0]);
	delete[] data;
	currentWindow->refresh();
}
//code dump
#if 0
void test()
{
	/*stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load("lena512.bmp", &col, &row, &row_width, 0);
	*/
	/*
	int cnt = row_width * row;
	int p = 0;
	unsigned char *data = new unsigned char [cnt];
	if (data == NULL) {
	printf("ooops");
	}
	for (int i = 0; i < cnt; i++) {
	data[i] = static_cast<unsigned char>(src->im[i]);
	}*/
	/*
	for (int i = 0; i < row; i++) {
	int j = 0;
	for (; j < col; j++) {
	data[p] = static_cast<unsigned char>(src->im[p]);
	p++;
	}
	}*/

	int cnt = row_width * row;
	int p = 0;
	unsigned char *data = new unsigned char[cnt];
	FUNC_TRAVERSE_PIXEL(src->im, Pixel, row, col, row_width, {
		data[p++] = imptr->B;
	data[p++] = imptr->G;
	data[p++] = imptr->R;
		});
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	figure("Hi", NULL);
	int col, row, row_width;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	unsigned char *data = stbi_load("lena508_510.bmp", &col, &row, &row_width, 4);
	currentWindow->diplayImage(data, row, col);//size[1], size[0]);
	delete[] data;
	currentWindow->refresh();
}
#endif

