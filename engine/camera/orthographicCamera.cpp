#include "orthographicCamera.h"

OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float near, float far) {
	mLeft = left;
	mRight = right;
	mBottom = bottom;
	mTop = top;
	mNear = near;
	mFar = far;
}

OrthographicCamera::~OrthographicCamera() {

}

glm::mat4 OrthographicCamera::getProjectionMatrix() const {
	float scale = std::pow(2, mScale);
	return glm::ortho(mLeft * scale, mRight * scale, mBottom * scale, mTop * scale, mNear, mFar);
}

void OrthographicCamera::scale(float deltaScale) {
	mScale += deltaScale;
}

