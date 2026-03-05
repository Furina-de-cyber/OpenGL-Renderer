#include "application.h"
#include <iostream>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

Application::Application() {

}

Application::~Application() {

}

void Application::test() {
	std::cout << "App Test" << std::endl;
}

int Application::getWidth()const {
	return mWidth;
}

int Application::getHeight()const {
	return mHeight;
}

bool Application::init(const int& width, const int& height) {
	mWidth = width;
	mHeight = height;
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	mWindow = glfwCreateWindow(mWidth, mHeight, "OpenGL Version", nullptr, nullptr);
	if (mWindow == NULL) {
		return false;
	}

	glfwMakeContextCurrent(mWindow);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	glfwSetFramebufferSizeCallback(mWindow, frameBufferResizeCallback);
	glfwSetKeyCallback(mWindow, funcKeyboardCallback);
	glfwSetMouseButtonCallback(mWindow, funcMouseCallback);
	glfwSetCursorPosCallback(mWindow, funcCursorCallback);
	glfwSetScrollCallback(mWindow, funcScrollCallback);

	glfwSetWindowUserPointer(mWindow, this);

	return true;
}

bool Application::update() {
	if (glfwWindowShouldClose(mWindow)) {
		return false;
	}
	glfwSwapBuffers(mWindow);
	glfwPollEvents();
	
	return true;
}

void Application::destroy() {
	glfwTerminate();
}

void Application::frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
	// if (Application::getInstance()->mResizeCallback != nullptr){
	// 	Application::getInstance()->mResizeCallback(width, height);
	// }
	Application* self = (Application*)glfwGetWindowUserPointer(window);
	if (self->mResizeCallback != nullptr) {
		self->mResizeCallback(width, height);
	}
}

void Application::funcKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Application* self = (Application*)glfwGetWindowUserPointer(window);
	if (self->mKeyboardCallback != nullptr) {
		self->mKeyboardCallback(key, action, mods);
	}
}

void Application::funcMouseCallback(GLFWwindow* window, int button, int action, int mods) {
	Application* self = (Application*)glfwGetWindowUserPointer(window);
	if (self->mMouseCallback != nullptr) {
		self->mMouseCallback(button, action, mods);
	}
}

void Application::funcCursorCallback(GLFWwindow* window, double xpos, double ypos) {
	Application* self = (Application*)glfwGetWindowUserPointer(window);
	if (self->mCursorCallback != nullptr) {
		self->mCursorCallback(xpos, ypos);
	}
}

void Application::funcScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	Application* self = (Application*)glfwGetWindowUserPointer(window);
	if (self->mScrollCallback != nullptr) {
		self->mScrollCallback(yoffset);
	}
}

void Application::getMouseCursor(double* xpos, double* ypos) {
	glfwGetCursorPos(mWindow, xpos, ypos);
}