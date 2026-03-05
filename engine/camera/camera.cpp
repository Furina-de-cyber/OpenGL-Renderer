#include "camera.h"

Camera::Camera() {

}

Camera::~Camera() {

}

glm::mat4 Camera::getViewMatrix() const{
	glm::vec3 front = glm::cross(mUp, mRight);
	glm::vec3 center = front + mPosition;

	return glm::lookAt(mPosition, center, mUp);
}

glm::mat4 Camera::getProjectionMatrix() const {
	return glm::mat4(1.0f);
}

void Camera::scale(float deltaScale) {

}