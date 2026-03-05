#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"

//#define STB_IMAGE_IMPLEMENTATION
#include "./resource/stb_image.h"
#include "./resource/stb_image_write.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp> 

#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <string>
#include <queue>
#include <exception>
#include <functional>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
using imagePtr = std::shared_ptr<unsigned char>;

const size_t BYTES_PER_VPTR = 8ull;

const float PI = 3.14159265359f;

const float FLOATINF = std::numeric_limits<float>::max();