#pragma once

#include <glm/glm.hpp> // for vectors and matrices for computer graphics

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};