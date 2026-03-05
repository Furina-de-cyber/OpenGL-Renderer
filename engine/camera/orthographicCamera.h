#pragma once

#include "camera.h"
#include "../../core.h"

class OrthographicCamera : public Camera {
public:
	OrthographicCamera(float left, float right, float bottom, float top, float near, float far);
	~OrthographicCamera();
	glm::mat4 getProjectionMatrix() const override;
	void scale(float deltaScale) override;

	float mLeft = 0.0f;
	float mRight = 0.0f;
	float mBottom = 0.0f;
	float mTop = 0.0f;
	float mNear = 0.0f;
	float mFar = 0.0f;

	float mScale = 0.0f;
};