#pragma once

#include "../../core.h"
#include "camera.h"
#include <map>

class CameraControl {
protected:
	bool mLeftMouseDown = false;
	bool mRightMouseDown = false;
	bool mMiddleMouseDown = false;

	double mCurrentX = 0, mCurrentY = 0;
	std::map<int, bool> mKeyMap;

	double mSensitivity = 0.1f;
	double mScaleSpeed = 0.2f;

	std::shared_ptr<Camera> mCamera;
public:
	CameraControl(std::shared_ptr<Camera> inCam)
		:mCamera{ inCam } {
	}
	~CameraControl() = default;

	virtual void update() {}
	virtual void onKeyboard(int key, int action, int mods);
	virtual void onMouse(int button, int action, double xpos, double ypos);
	virtual void onCursor(double xpos, double ypos) {}
	virtual void onScroll(double yoffset) {}
};