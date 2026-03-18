#pragma once

#include <iostream>
#include <stdexcept>
#include <cstdlib> // For uint32_t
#include <limits> // for std::numeric_limits
#include <algorithm> // for std::clamp
#include <fstream> // read shader file

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
//#define VULKAN_HPP_NO_EXCEPTIONS
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

    GLFWwindow * window {nullptr};

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

    vk::raii::Device device {nullptr}; // logical device

    uint32_t queueIndex;
    vk::raii::Queue queue {nullptr};

    vk::raii::SurfaceKHR surface {nullptr};

    vk::Extent2D swapChainExtent;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::raii::SwapchainKHR swapChain {nullptr};
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::raii::ImageView> swapChainImageViews;

    vk::raii::PipelineLayout pipelineLayout {nullptr};
    vk::raii::Pipeline graphicsPipeline {nullptr};

    vk::raii::CommandPool commandPool {nullptr};
    vk::raii::CommandBuffer commandBuffer {nullptr};

    /////////////////////////////////////// METHODS //////////////////////////////////////////////////

    void initVulkan();
    void mainLoop();
    void cleanup();

    void initWindow();

    std::vector<char const *> getRequiredGLFWExtensions() const;
    std::vector<char const *> getRequiredValidationLayers() const;
    void createInstance();

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT        severity,
        vk::DebugUtilsMessageTypeFlagsEXT               type,
        vk::DebugUtilsMessengerCallbackDataEXT const *  pCallBackData,
        void *                                          pUserData
    );

    void setupDebugMessenger();

    void pickPhysicalDevice();
    bool isDeviceSuitable(vk::raii::PhysicalDevice const & physicalDevice) const;

    void createLogicalDevice();

    void createSurface();

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const & availableFormats) const;
    vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const & availablePresentModes) const;
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const & capabilities) const;
    uint32_t chooseSwapImageCount(vk::SurfaceCapabilitiesKHR const & surfaceCapabilities) const;
    void createSwapChain();

    void createImageViews();

    void createGraphicsPipeline();

    static std::vector<char> readFile(std::string const & filename);

    [[nodiscard]] vk::raii::ShaderModule createShaderModule(std::vector<char> const & code) const;

    void createCommandPool();

    void createCommandBuffer();
};