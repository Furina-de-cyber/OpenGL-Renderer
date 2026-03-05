#pragma once

#include "cameraControl.h"

class GameCameraControl : public CameraControl {
public:
	GameCameraControl(std::shared_ptr<Camera> inCam)
		:CameraControl(inCam) {
	}
	~GameCameraControl() = default;

	void setKeySpeed(float value) { mKeySpeed = value; }
	void onCursor(double xpos, double ypos)override;
	void update()override;
private:
	float mDeltaYSum = 0.0f;
	float mMoveSpeed = 0.005f;
	float mKeySpeed = 0.001f;

	void pitch(float deltaY);
	void yaw(float deltaX);
};