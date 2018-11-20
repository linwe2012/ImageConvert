#pragma once
#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <functional>
#include <map>
#include "shader.h"
#include "camera.h"



#define GLFLOAT(X) static_cast<float>(X)

class Window
{
public:
	Window() :hasImage(FALSE), camera(NULL) {};
	Window(const char *windowTitle,int width, int height, int anti_aliasing, int camera_status)
		:hasImage(FALSE)
	{
		create(windowTitle, width, height, anti_aliasing, camera_status);
	}

	void create(const char *windowTitle, int width, int height, int anti_aliasing, int camera_status)
	{
		if (anti_aliasing == TRUE) {
			glfwWindowHint(GLFW_SAMPLES, 4);
		}
#ifdef  __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //something for mac
#endif //  __APPLE__

		window = glfwCreateWindow(width, height, windowTitle, NULL, NULL);
		if (window == NULL)
		{
			printf("Failed to create GLFW window");
			glfwTerminate();
		}
		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
		// glfwSetWindowSizeCallback(window, window_size_callback);
		glfwSetKeyCallback(window, glim_key_callback);
		// glfwSetCursorPosCallback(window, glim_mouse_callback);
		scroll_map[window] = this;
		glfwSetScrollCallback(window, glim_scroll_callback);
		glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			printf("Failed to initialize GLAD");
		}
		/*
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		*/
		// glEnable(GL_DEPTH_TEST);
		if (camera_status) {
			startCamera();
		}
		camera = new Camera();
		camera->adjustMovementStyle(camera_status);
		allowCamera = camera_status;
		this->camera_status = camera_status;
		lastX = width / 2.0f;
		lastY = height / 2.0f;
		win_width = width;
		win_height = height;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
	}
	void initShader(int shader_status)
	{
		this->shader_status = shader_status;
		std::vector<std::string> mats;
		std::vector<std::string> texs;
		if(shader_status & SHADER_TEXTURE)
			texs.push_back("ourTexture");
		
		if (shader_status & SHADER_CAMERA)
		{
			mats.push_back("view");
			mats.push_back("projection");
			hasCamera = TRUE;
		}
		const char *vs = shader.genVSCode(mats, texs);
		const char *fs = shader.genFSCode(texs);
		shader.compile(vs, fs);
		delete[] vs;
		delete[] fs;
	}
	void diplayImage(unsigned char *data, int width, int height)
	{
		float sz = 0.5f, dp = -1.0f;
		float vertices[] = {
			// positions       // colors       // texture coords
			 2 * sz, sz , dp,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
			 2 * sz, -sz, dp,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
			-2 * sz, -sz, dp,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
			-2 * sz,  sz, dp,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
		};

		unsigned int indices[] = {
			0, 1, 3, // first triangle
			1, 2, 3  // second triangle
		};
		texture.push_back(-1);
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
		if (!(shader_status & SHADER_TEXTURE)) {
			printf("Window::diplayImage::shader without texture. Please re-compile shader with textures.\n");
		}
		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		// texture coord attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		for (int i = 0, len = texture.size(); i < len; i++) {
			glGenTextures(1, &texture[i]);
			glBindTexture(GL_TEXTURE_2D, texture[i]);
			//do not repeat & linear
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		shader.use();
		shader.setInt("ourTexture", 0);
		hasImage = 1;
	}
	void refresh()
	{
		glfwMakeContextCurrent(window);
		if (!glfwWindowShouldClose(window)) {
			shader.use();
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			processInput();

			//compute delta time of this window between 2 frames
			currentFrame = GLFLOAT(glfwGetTime());
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;

			
			// if (allowCamera)
			processCamera();

			if (hasImage)
				renderImage(0);
			
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		else if (window)
			close();
	}
	void close()
	{
		glfwDestroyWindow(window);
		window = NULL;
	}
	void renderImage(int textcnt)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture[textcnt]);
		// shader.use();
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
	void startCamera()
	{
		allowCamera = true;
		firstMouse = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	void cancelCamera(int Mode)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		allowCamera = false;
		if (Mode == 0) {
			camera->BudgeCamera(0.0f, 0.7f);
		}
	}
	GLFWwindow* window;
	Shader shader;
	int shader_status;

	int hasImage;
	std::vector<unsigned int> texture;
	unsigned int VBO, VAO, EBO;

	Camera *camera = NULL;
	int allowCamera = TRUE;
	int camera_status;
	int hasCamera = FALSE;
	float deltaTime = 0.0f;	// time between current frame and last frame
	float lastFrame = 0.0f;
	float currentFrame;
	float lastX;
	float lastY;
	int win_width, win_height;
	bool firstMouse = true;
	static std::map<GLFWwindow*, Window *> scroll_map;
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}
	static void glim_key_callback(GLFWwindow* mwindow, int key, int scancode, int action, int mods)
	{
		printf("h");
	}

	static void glim_mouse_callback(GLFWwindow* window, double xpos, double ypos) {

	}
	static void glim_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
		scroll_map[window]->camera->ProcessMouseScroll(GLFLOAT(yoffset));
	}
private:
	double mouse_xpos, mouse_ypos;
	void camera_mouse_process()
	{
		if (firstMouse)
		{
			lastX = static_cast<float>(mouse_xpos);
			lastY = static_cast<float>(mouse_ypos);
			firstMouse = false;
		}

		float xoffset = static_cast<float>(mouse_xpos) - lastX;
		float yoffset = lastY - static_cast<float>(mouse_ypos); // reversed since y-coordinates go from bottom to top

		lastX = static_cast<float>(mouse_xpos);
		lastY = static_cast<float>(mouse_ypos);

		camera->ProcessMouseMovement(xoffset, yoffset);
	}
	void processInput()
	{
		static double LastTime;
		static int escapeMutex = TRUE;
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			if (escapeMutex) {
				// printf("AllowCam=%d ", allowCamera);
				if (allowCamera) {
					allowCamera = FALSE;
					cancelCamera(0);
				}
				else
				{
					// glfwSetWindowShouldClose(window, true);
				}
			}
			escapeMutex = FALSE;
		}
		else {
			escapeMutex = TRUE;
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		{
			if (!allowCamera) {
				startCamera();
			}
		}
		if (allowCamera) {
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
				camera->ProcessKeyboard(FORWARD, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
				camera->ProcessKeyboard(BACKWARD, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
				camera->ProcessKeyboard(LEFT, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
				camera->ProcessKeyboard(RIGHT, deltaTime);
		}
		glfwGetCursorPos(window, &mouse_xpos, &mouse_ypos);
	}
	void processCamera()
	{

		//pass delta time to determine if it should move back
		camera->PassDeltaTime(deltaTime);
		// pass projection matrix to shader (note that in this case it could change every frame)
		glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), (float)win_width / (float)win_height, 0.1f, 100.0f);
		shader.setMat4("projection", projection);
		
		// camera/view transformation
		glm::mat4 view = camera->GetViewMatrix();
		shader.setMat4("view", view);

		if (!(camera_status & CAMERA_HORIZONTAL) && allowCamera) {
			camera_mouse_process();
		}
	}
};
std::map<GLFWwindow*, Window *> Window::scroll_map;

//code dump
#if 0
if (shader_status & SHADER_CAMERA)
{
	vs = "#version 330 core\n"
		"layout(location = 0) in vec3 aPos;\n"
		"layout(location = 1) in vec3 aColor;\n"
		"layout(location = 2) in vec2 aTexCoord;\n"
		"out vec3 ourColor;\n"
		"out vec2 TexCoord;\n"
		"uniform mat4 view;\n"
		"uniform mat4 projection;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = projection * view * vec4(aPos, 1.0);\n"
		"	TexCoord = aTexCoord;\n"
		"	ourColor = aColor;\n"
		"}";

	fs =
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"\nin vec3 ourColor;"
		"\nin vec2 TexCoord;"
		"\nuniform sampler2D ourTexture;"
		"\nvoid main()"
		"\n{"
		"\n	 FragColor = texture(ourTexture, TexCoord);"
		"\n}";
	printf("vertex shader=\n%s,\n\nfrgment shader=\n%s,\n", vs, fs);
}
#endif