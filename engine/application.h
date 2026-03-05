#pragma once

#include <cstdint>

using resizeCallback = void(*)(int width, int height);
using keyboardCallback = void(*)(int key, int action, int mods);
using mouseCallback = void(*)(int button, int action, int mods);
using cursorCallback = void(*)(double xpos, double ypos);
using scrollCallback = void(*)(double yoffset);

class GLFWwindow;

class Application {
public:
	Application();
	~Application();
	void test();
	int getWidth() const;
	int getHeight() const;

	bool init(const int& width, const int& height);
	bool update();
	void destroy();

	void getMouseCursor(double* xpos, double* ypos);

	void setResizeCallback(resizeCallback callback) { mResizeCallback = callback; }
	void setKeyboardCallback(keyboardCallback callback) { mKeyboardCallback = callback; }
	void setMouseCallback(mouseCallback callback) { mMouseCallback = callback; }
	void setCursorCallback(cursorCallback callback) { mCursorCallback = callback; }
	void setScrollCallback(scrollCallback callback) { mScrollCallback = callback; }

private:
	int mWidth{ 0 };
	int mHeight{ 0 };
	GLFWwindow* mWindow{ nullptr };

	resizeCallback mResizeCallback{ nullptr };
	keyboardCallback mKeyboardCallback{ nullptr };
	mouseCallback mMouseCallback{ nullptr };
	cursorCallback mCursorCallback{ nullptr };
	scrollCallback mScrollCallback{ nullptr };

	static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
	static void funcKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void funcMouseCallback(GLFWwindow* window, int button, int action, int mods);
	static void funcCursorCallback(GLFWwindow* window, double xpos, double ypos);
	static void funcScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};