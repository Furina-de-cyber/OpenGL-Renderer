#include "gameCameraControl.h"

void GameCameraControl::onCursor(double xpos, double ypos) {
	if (mLeftMouseDown) {
		float deltaX = float((mCurrentX - xpos) * mSensitivity);
		float deltaY = float((mCurrentY - ypos) * mSensitivity);

		pitch(deltaY);
		yaw(deltaX);
	}
	else if (mMiddleMouseDown) {
		float deltaX = float((mCurrentX - xpos) * mMoveSpeed);
		float deltaY = -float((mCurrentY - ypos) * mMoveSpeed);

		mCamera->mPosition += mCamera->mRight * deltaX;
		mCamera->mPosition += mCamera->mUp * deltaY;
	}

	mCurrentX = xpos;
	mCurrentY = ypos;
}

void GameCameraControl::pitch(float deltaY) {
	mDeltaYSum += deltaY;
	if (mDeltaYSum > 89.0f || mDeltaYSum < -89.0f) {
		mDeltaYSum -= deltaY;
		return;
	}
	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(deltaY), mCamera->mRight);

	mCamera->mUp = rotate * glm::vec4(mCamera->mUp, 0.0f);
}

void GameCameraControl::yaw(float deltaX) {
	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(deltaX), glm::vec3(0.0f, 1.0f, 0.0f));

	mCamera->mUp = rotate * glm::vec4(mCamera->mUp, 0.0f);
	mCamera->mRight = rotate * glm::vec4(mCamera->mRight, 0.0f);
}

void GameCameraControl::update() {
	glm::vec3 direction(0.0f, 0.0f, 0.0f);

	glm::vec3 front = glm::cross(mCamera->mUp, mCamera->mRight);
	glm::vec3 right = mCamera->mRight;

	if (mKeyMap[GLFW_KEY_W]) {
		direction += front;
	}
	if (mKeyMap[GLFW_KEY_S]) {
		direction -= front;
	}
	if (mKeyMap[GLFW_KEY_D]) {
		direction += right;
	}
	if (mKeyMap[GLFW_KEY_A]) {
		direction -= right;
	}
	if (glm::length(direction) != 0) {
		direction = glm::normalize(direction);
		mCamera->mPosition += direction * mKeySpeed;
	}
}