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
    ///////////////////////////////////////////////// MEMBER VARIABLES //////////////////////////////////
    uint32_t const WIDTH {800};
    uint32_t const HEIGHT {600};

    GLFWwindow * window;

    vk::raii::Context context;
    vk::raii::Instance instance {nullptr};

    #ifdef NDEBUG
    const bool enableValidationLayers {false};
    #else
    const bool enableValidationLayers {true};
    #endif

    std::vector<char const *> const validationLayers {
        "VK_LAYER_KHRONOS_validation"
    };

    vk::raii::DebugUtilsMessengerEXT debugMessenger {nullptr};

    vk::raii::PhysicalDevice physicalDevice {nullptr};

    /////////////////////////////////////// METHODS //////////////////////////////////////////////////

    void initVulkan();
    void mainLoop();
    void cleanup();

    void initWindow();

    std::vector<char const *> getRequiredGLFWExtensions();
    std::vector<char const *> getRequiredValidationLayers();
    void createInstance();

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT        severity,
        vk::DebugUtilsMessageTypeFlagsEXT               type,
        vk::DebugUtilsMessengerCallbackDataEXT const *  pCallBackData,
        void *                                          pUserData
    );

    void setupDebugMessenger();

    void pickPhysicalDevice();
    bool isDeviceSuitable(vk::raii::PhysicalDevice const & physicalDevice);
};