#pragma once

#include "../../core.h"

class Camera {
public:
	Camera();
	~Camera();
	
	glm::mat4 getViewMatrix() const;
	glm::mat4 getInvViewMatrix() const {
		return glm::inverse(getViewMatrix());
	}
	virtual glm::mat4 getProjectionMatrix() const;
	virtual void scale(float deltaScale);

	glm::vec3 mPosition = { 0.0f, 0.0f, 1.0f };
	glm::vec3 mRight = { 1.0f, 0.0f, 0.0f };
	glm::vec3 mUp = { 0.0f,1.0f,0.0f };

	virtual const float getFovy() const { return 0.0f; }
	virtual const float getAspect() const { return 0.0f; }
};

struct FCameraInfo {
	glm::mat4 mProjection;
	glm::mat4 mView;
	glm::vec4 mPosition;
	FCameraInfo(glm::mat4 inProj, glm::mat4 inView, glm::vec4 inPos) 
		: mProjection{inProj}, mView{inView}, mPosition{inPos}
	{}
	FCameraInfo() = default;
};

inline FCameraInfo getCameraInfo(std::shared_ptr<Camera> camera) {
	return FCameraInfo(camera->getProjectionMatrix(), 
		camera->getViewMatrix(), glm::vec4(camera->mPosition,1.0f));
}
