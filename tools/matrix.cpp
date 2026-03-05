#include "matrix.h"

glm::mat4 BuildBoxModelMatrix(
    const glm::vec4& mPosition,
    const glm::vec4& mDirection,
    const glm::vec4& mRight
) {
    glm::vec3 pos = glm::vec3(mPosition);

    glm::vec3 dir = glm::normalize(glm::vec3(mDirection));
    glm::vec3 right = glm::normalize(glm::vec3(mRight));
    right = glm::normalize(right - dir * glm::dot(right, dir));

    glm::vec3 forward = glm::normalize(glm::cross(right, dir));

    glm::mat4 model(1.0f);
    model[0] = glm::vec4(right, 0.0f);
    model[1] = glm::vec4(dir, 0.0f);
    model[2] = glm::vec4(forward, 0.0f);
    model[3] = glm::vec4(pos, 1.0f);

    return model;
}
