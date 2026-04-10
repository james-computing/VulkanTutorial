#pragma once

#include <glm/glm.hpp> // for vectors and matrices for computer graphics

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};