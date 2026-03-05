#pragma once

#include "../core.h"
#include "LTC.h"
glm::mat4 BuildBoxModelMatrix(
    const glm::vec4& mPosition,
    const glm::vec4& mDirection,
    const glm::vec4& mRight
);