#pragma once

#include "camera.h"
#include "../../core.h"

class PerspectiveCamera : public Camera {
public:
	PerspectiveCamera(float fovy, float aspect, float near, float far);
	~PerspectiveCamera();
	glm::mat4 getProjectionMatrix() const override;
	void scale(float deltaScale)override;
	const float getFovy() const override { return mFovy; }
	const float getAspect() const override { return mAspect; }
private:
	float mFovy = 0.0f;
	float mAspect = 0.0f;
	float mNear = 0.0f;
	float mFar = 0.0f;
};