#include "cameraControl.h"
#include "../../core.h"

void CameraControl::onMouse(int button, int action, double xpos, double ypos) {
	bool pressed = (action == GLFW_PRESS) ? true : false;

	if (pressed) {
		mCurrentX = xpos;
		mCurrentY = ypos;
	}

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		mLeftMouseDown = pressed;
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		mRightMouseDown = pressed;
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		mMiddleMouseDown = pressed;
		break;
	}
}

void CameraControl::onKeyboard(int key, int action, int mods) {
	if (action == GLFW_REPEAT) {
		return;
	}
	bool pressed = (action == GLFW_PRESS) ? true : false;
	mKeyMap[key] = pressed;
}