#pragma once

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_EXCEPTIONS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Application {
public:
    void run();
private:
    uint32_t const WIDTH {800};
    uint32_t const HEIGHT {600};

    GLFWwindow * window;

    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;

    #ifdef NDEBUG
    const bool enableValidationLayers {false};
    #else
    const bool enableValidationLayers {true};
    #endif

    std::vector<char const *> const validationLayers {
        "VK_LAYER_KHRONOS_validation"
    };

    void initVulkan();
    void mainLoop();
    void cleanup();

    void initWindow();

    void getRequiredGLFWExtensions(uint32_t & glfwExtensionCount, char const ** & glfwExtensions);
    void getRequiredValidationLayers(std::vector<char const *> & requiredValidationLayers);
    void createInstance();
};