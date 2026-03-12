#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Application {
public:
    void run();
private:
    uint32_t const WIDTH = 800;
    uint32_t const HEIGHT = 600;

    GLFWwindow * window;

    void initVulkan();
    void mainLoop();
    void cleanup();
    void initWindow();
};