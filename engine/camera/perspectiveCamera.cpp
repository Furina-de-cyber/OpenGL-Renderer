#include "perspectiveCamera.h"

PerspectiveCamera::PerspectiveCamera(float fovy, float aspect, float near, float far) 
	:mFovy{fovy}, mAspect{aspect}, mNear{near}, mFar{far}
{}

PerspectiveCamera::~PerspectiveCamera() {

}

glm::mat4 PerspectiveCamera::getProjectionMatrix() const{
	return glm::perspective(glm::radians(mFovy), mAspect, mNear, mFar);
}

void PerspectiveCamera::scale(float deltaScale) {
	glm::vec3 front = glm::cross(mUp, mRight);
	mPosition = mPosition + front * deltaScale;
}