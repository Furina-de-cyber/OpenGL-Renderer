#pragma once

#include "cameraControl.h"

class TrackBallCameraControl : public CameraControl {
public:
	TrackBallCameraControl(std::shared_ptr<Camera> inCam)
		:CameraControl(inCam) {
	}
	~TrackBallCameraControl() = default;

	void onCursor(double xpos, double ypos)override;
	void onScroll(double yoffset)override;
private:
	float mDeltaYSum = 0.0f;
	float mMoveSpeed = 0.005f;

	void pitch(float deltaY);
	void yaw(float deltaX);
};