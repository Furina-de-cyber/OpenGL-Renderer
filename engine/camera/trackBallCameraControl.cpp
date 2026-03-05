#include "trackBallCameraControl.h"

void TrackBallCameraControl::onCursor(double xpos, double ypos) {
	if (mLeftMouseDown) {
		float deltaX = float((mCurrentX - xpos) * mSensitivity);
		float deltaY = float((mCurrentY - ypos) * mSensitivity);

		pitch(deltaY);
		yaw(deltaX);
	}
	else if (mMiddleMouseDown) {
		float deltaX = float((mCurrentX - xpos) * mMoveSpeed);
		float deltaY = - float((mCurrentY - ypos) * mMoveSpeed);

		mCamera->mPosition += mCamera->mRight * deltaX;
		mCamera->mPosition += mCamera->mUp * deltaY;
	}

	mCurrentX = xpos;
	mCurrentY = ypos;
}

void TrackBallCameraControl::pitch(float deltaY) {
	mDeltaYSum += deltaY;
	//if (mDeltaYSum > 89.0f || mDeltaYSum < -89.0f) {
	//	mDeltaYSum -= deltaY;
	//	return;
	//}
	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(deltaY), mCamera->mRight);

	mCamera->mPosition = rotate * glm::vec4(mCamera->mPosition, 1.0f);
	mCamera->mUp = rotate * glm::vec4(mCamera->mUp, 0.0f);
}

void TrackBallCameraControl::yaw(float deltaX) {
	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(deltaX), mCamera->mUp);

	mCamera->mPosition = rotate * glm::vec4(mCamera->mPosition, 1.0f);
	mCamera->mUp = rotate * glm::vec4(mCamera->mUp, 0.0f);
	mCamera->mRight = rotate * glm::vec4(mCamera->mRight, 0.0f);
}

void TrackBallCameraControl::onScroll(double yoffset) {
	mCamera->scale(yoffset * mScaleSpeed);
}