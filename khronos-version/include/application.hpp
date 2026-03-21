#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
//#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib> // For uint32_t
#include <limits> // for std::numeric_limits
#include <algorithm> // for std::clamp
#include <fstream> // read shader file
#include <glm/gtc/matrix_transform.hpp> // for model view projection
#include <chrono> // for model view projection
#include "../include/vertex.hpp"
#include "../include/mvp.hpp"

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
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    uint32_t const MAX_FRAMES_IN_FLIGHT {2};
    uint32_t frameIndex {0};

    bool frameBufferResized {false};

    std::vector<Vertex> const vertices {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    std::vector<uint16_t> const indices {
        0, 1, 2, 2, 3, 0
    };

    vk::raii::Buffer vertexBuffer {nullptr};
    vk::raii::DeviceMemory vertexBufferMemory {nullptr};
    vk::raii::Buffer indexBuffer {nullptr};
    vk::raii::DeviceMemory indexBufferMemory {nullptr};

    vk::raii::DescriptorSetLayout descriptorSetLayout {nullptr}; // for model view projection, which uses uniform buffers

    std::vector<vk::raii::Buffer> uniformBuffers; // model view projection matrices are stored in uniform buffers
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemories;
    std::vector<void*> uniformBuffersMapped; // pointers to transfer data from host to uniform buffers

    vk::raii::DescriptorPool descriptorPool {nullptr};
    std::vector<vk::raii::DescriptorSet> descriptorSets;

    std::string const texturePath {"../Rendering/textures/texture.jpg"};

    vk::raii::Image textureImage {nullptr};

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

    void createCommandBuffers();

    void transitionImageLayout(
        uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask
    ) const;

    void recordCommandBuffer(uint32_t imageIndex);

    void createSyncObjects();

    void drawFrame();

    void cleanupSwapChain();
    void recreateSwapChain();

    static void frameBufferResizeCallback(GLFWwindow * window, int width, int height);

    void createVertexBuffer();

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    void createBuffer(
        vk::DeviceSize bufferSize,
        vk::BufferUsageFlags bufferUsage,
        vk::MemoryPropertyFlags memoryProperties,
        vk::raii::Buffer & buffer,
        vk::raii::DeviceMemory & bufferMemory
    );

    void copyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize bufferSize) const;

    void createIndexBuffer();

    void createDescriptorSetLayout();
    void createUniformBuffers();
    
    void updateUniformBuffer(uint32_t currentImage);

    void createDescriptorPool();
    void createDescriptorSets();

    void createTextureImage();
    void createImage(
        uint32_t width,
        uint32_t height,
        vk::Format imageFormat,
        vk::ImageTiling imageTiling,
        vk::ImageUsageFlags imageUsage,
        //vk::MemoryPropertyFlags imageMemoryProperties,
        vk::raii::Image & image//,
        //vk::raii::DeviceMemory & imageMemory
    ) const;
};