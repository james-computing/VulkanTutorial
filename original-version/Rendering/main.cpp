#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS // for glm::rotate
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GML_ENABLE_EXPERIMENTAL
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring> //for strcmp, to compare C strings
#include <optional>
#include <set>
#include <cstdint> // for uint32_t
#include <limits> // for std::numeric_limits
#include <algorithm> // for std::clamp
#include <fstream>
#include <array>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#include "libraries/stb/stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "libraries/tinyobjloader/tiny_obj_loader.h"
#include <unordered_map>

// Validation layers
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
    const bool enableValidationLayers {false};
#else
    const bool enableValidationLayers {true};
#endif


struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};


// Swap chains
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

const size_t MAX_FRAMES_IN_FLIGHT {2};

// For vertex buffer
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 textureCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, textureCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex & other) const
    {
        return pos == other.pos &&
                color == other.color &&
                textureCoord == other.textureCoord;
    }
};

namespace std
{
    template <>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const & vertex) const
        {
            return (
                        (
                            hash<glm::vec3>()(vertex.pos) ^
                            (hash<glm::vec3>()(vertex.color) << 1)
                        ) >> 1
                    )
                    ^ (hash<glm::vec2>()(vertex.textureCoord) << 1);
        }
    };
}

// Model
const std::string MODEL_PATH {"models/viking_room.obj"};
const std::string TEXTURE_PATH {"textures/viking_room.png"};

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class Application
{
private:
    // GLFWwindow
    GLFWwindow * window;
    const uint32_t WIDTH {800};
    const uint32_t HEIGHT {600};

    // Vulkan instance
    VkInstance vkInstance;

    // Window surface
    VkSurfaceKHR surface;

    // Physical device
    VkPhysicalDevice vkPhysicalDevice {VK_NULL_HANDLE};

    // Queue families
    QueueFamilyIndices queueFamilyIndices;

    // Logical device, to interface with the physical device
    VkDevice vkDevice;

    // Graphics queue
    VkQueue graphicsQueue;

    // Presentation queue
    VkQueue presentQueue;

    // Swap chain
    VkSwapchainKHR swapChain;

    // Images
    std::vector<VkImage> swapChainImages;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    // Image views
    std::vector<VkImageView> swapChainImageViews;

    // Render pass
    VkRenderPass renderPass;

    // Descriptor set layout
    VkDescriptorSetLayout descriptorSetLayout;

    // Pipeline layout
    VkPipelineLayout pipelineLayout;

    // Pipeline
    VkPipeline pipeline;

    // Framebuffers
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Command pool
    VkCommandPool commandPool;
    
    // Command buffer
    std::vector<VkCommandBuffer> commandBuffers;

    // Syncing
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    // Handling resizing explicitly
    bool framebufferResized {false};

    // Frame flight
    uint32_t currentFrame {0};

    // Vertices data
    std::vector<Vertex> vertices;
    std::vector<uint32_t> vertexIndices;

    // Vertex buffer
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    // Index buffer
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // Uniform buffers
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;

    // Descriptor pool
    VkDescriptorPool descriptorPool;

    // Descriptor sets
    std::vector<VkDescriptorSet> descriptorSets;

    // Texture
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    uint32_t mipLevels;

    // Depth
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // Multisampling
    VkSampleCountFlagBits msaaSamples {VK_SAMPLE_COUNT_1_BIT}; // initialize with no multisampling
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    static void framebufferResizeCallback(GLFWwindow * window, int width, int height)
    {
        Application * app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initWindow()
    {
        glfwInit();

        // Don't create context for OpenGL, since we are using Vulkan
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Disable window resizing
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Create a window
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        // Pass this object to GLFW, so we can reference it from the callback
        glfwSetWindowUserPointer(window, this);
        // Set a callback to be called when window is resized
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void initVulkan()
    {
        std::cout << "create instance" << std::endl;
        createVkInstance();
        std::cout << "create surface" << std::endl;
        createSurface();
        std::cout << "create physical device" << std::endl;
        pickPhysicalDevice();
        std::cout << "create logical device" << std::endl;
        createLogicalDevice();
        std::cout << "create swap chain" << std::endl;
        createSwapChain();
        std::cout << "create image views" << std::endl;
        createImageViews();
        std::cout << "create render pass" << std::endl;
        createRenderPass();
        std::cout << "create descriptor set layout" << std::endl;
        createDescriptorSetLayout();
        std::cout << "create graphics pipeline" << std::endl;
        createGraphicsPipeline();
        std::cout << "create color resources" << std::endl;
        createColorResources();
        std::cout << "create depth resources" << std::endl;
        createDepthResources();
        std::cout << "create framebuffers" << std::endl;
        createFramebuffers();
        std::cout << "create command pool" << std::endl;
        createCommandPool();
        std::cout << "create texture image" << std::endl;
        createTextureImage();
        std::cout << "create texture image view" << std::endl;
        createTextureImageView();
        std::cout << "create texture sampler" << std::endl;
        createTextureSampler();
        std::cout << "load model" << std::endl;
        loadModel();
        std::cout << "create vertex buffer" << std::endl;
        createVertexBuffer();
        std::cout << "create index buffer" << std::endl;
        createIndexBuffer();
        std::cout << "create uniform buffers" << std::endl;
        createUniformBuffers();
        std::cout << "create descriptor pools" << std::endl;
        createDescriptorPool();
        std::cout << "create descriptor sets" << std::endl;
        createDescriptorSets();
        std::cout << "create command buffer" << std::endl;
        createCommandBuffers();
        std::cout << "create sync objects" << std::endl;
        createSyncObjects();
    }

    void createVkInstance()
    {
        if(enableValidationLayers)
        {
            bool hasValidationSupport = checkValidationLayerSupport();
            if(!hasValidationSupport)
            {
                throw std::runtime_error("Validation layers requested, but not available");
            }
        }

        VkApplicationInfo appInfo {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "No engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instanceCreateInfo {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount {0};
        const char ** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        instanceCreateInfo.enabledExtensionCount = glfwExtensionCount;
        instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions;

        if(enableValidationLayers)
        {
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            instanceCreateInfo.enabledLayerCount = 0;
        }

        VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &vkInstance);
        if(result != VK_SUCCESS)
        {
            std::cout << "Failed to create Vulkan instance" << std::endl;
        }
    }

    bool checkValidationLayerSupport()
    {
        // Count the number of validation layers available
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        // Initialize a vector of VkLayerProperties with layerCount entries
        std::vector<VkLayerProperties> availableLayers(layerCount);
        // Store the validation layer properties in the vector
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        std::cout << "available layers:\n";
        for(const VkLayerProperties& layerProperties : availableLayers)
        {
            std::cout << layerProperties.layerName << "\n";
        }
        std::cout << std::endl;

        // Check if the validation layers are available
        for(const char * layerName : validationLayers)
        {
            bool foundLayer = false;
            // search for layer in availableLayers
            for(const VkLayerProperties& layerProperties : availableLayers)
            {
                // if found layer, break and search for the next layer
                if(strcmp(layerName,layerProperties.layerName) == 0)
                {
                    foundLayer = true;
                    break;
                }
            }
            if(!foundLayer)
            {
                return false;
            }
        }

        return true;
    }

    void createSurface()
    {
        // nullptr refers to the custom allocator
        VkResult result = glfwCreateWindowSurface(vkInstance, window, nullptr, &surface);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface");
        }
    }

    void pickPhysicalDevice()
    {
        // Count the devices with Vulkan support
        uint32_t deviceCount {0};
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
        if(deviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support");
        }
        
        // Initialize a vector of VkPhysicalDevice with deviceCount entries
        std::vector<VkPhysicalDevice> devices(deviceCount);
        // Store the devices with Vulkan support into the devices vector
        vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

        // select the first suitable device
        for(VkPhysicalDevice device : devices)
        {
            if(isDeviceSuitable(device))
            {
                vkPhysicalDevice = device;
                msaaSamples = getMaxUsableSampleCount();
                std::cout << "msaaSamples = " << msaaSamples << std::endl;
                break;
            }
        }

        if(vkPhysicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkPhysicalDeviceFeatures physicalDeviceFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

        //bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        bool supportsGeometryShaders = physicalDeviceFeatures.geometryShader;
        
        std::cout << std::boolalpha;
        //std::cout << "-- is discrete gpu: " << isDiscreteGPU << std::endl;
        std::cout << "-- has geometry shaders: " << supportsGeometryShaders << std::endl;

        // Queue families
        queueFamilyIndices = findQueueFamilies(physicalDevice);

        std::cout << "-- has queue families: " << queueFamilyIndices.isComplete() << std::endl;

        // Swap chains
        bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);
        bool swapChainAdequate {false};
        if(extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(physicalDevice);
            swapChainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
        }

        bool isSuitable =  supportsGeometryShaders &&
                            queueFamilyIndices.isComplete() &&
                            swapChainAdequate && 
                            physicalDeviceFeatures.samplerAnisotropy;

        std::cout << "maxFramebufferWidth = " << physicalDeviceProperties.limits.maxFramebufferWidth << std::endl;
        std::cout << "maxFramebufferHeight = " << physicalDeviceProperties.limits.maxFramebufferHeight << std::endl;

        return isSuitable;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount {0};
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i {0};
        for(const VkQueueFamilyProperties& queueFamily : queueFamilies)
        {
            // Graphics family
            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            // Presentation family
            VkBool32 presentSupport {false};
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if(presentSupport)
            {
                indices.presentFamily = i;
            }

            // Early stop
            if(indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        // copy the deviceExtensions vector to a new requiredExtensions set
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        // remove from the set the extensions which are available
        for(const VkExtensionProperties& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        // if all extensions were removed, then all required extensions are available
        return requiredExtensions.empty();
    }

    void createLogicalDevice()
    {
        // Create the queues (graphics and presentation)
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};
        const float queuePriority = 1.0f;
        for(uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        // again?
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if(enableValidationLayers)
        {
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            deviceCreateInfo.enabledLayerCount = 0;
        }

        VkResult result = vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, nullptr, &vkDevice);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device");
        }

        // Get a queue handle
        vkGetDeviceQueue(vkDevice, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(vkDevice, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice)
    {
        SwapChainSupportDetails details;

        // Capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

        // Formats
        uint32_t formatCount {0};
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        if(formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        // Present mode
        uint32_t presentModeCount {0};
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if(presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());

        }

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & availableFormats)
    {
        // First check for preferable format
        for(const VkSurfaceFormatKHR & availableFormat : availableFormats)
        {
            if(
                availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            )
            {
                return availableFormat;
            }
        }

        // If the preferable format isn't avaible, pick the first one available
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> & availablePresentModes)
    {
        for(const VkPresentModeKHR & availablePresentMode : availablePresentModes)
        {
            if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
    {
        if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            std::cout << "capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()" << std::endl;
            return capabilities.currentExtent;
        }
        else
        {
            std::cout << "capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()" << std::endl;
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent =
            {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height),
            };

            actualExtent.width = std::clamp(
                                            actualExtent.width,
                                            capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width
                                        );
            actualExtent.width = std::clamp(
                                            actualExtent.height,
                                            capabilities.minImageExtent.height,
                                            capabilities.maxImageExtent.height
                                        );

            return actualExtent;
        }
    }

    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(vkPhysicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupportDetails.presentModes);
        swapChainExtent = chooseSwapExtent(swapChainSupportDetails.capabilities);

        swapChainImageFormat = surfaceFormat.format;

        // Choose image count as minimum + 1
        uint32_t minImageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
        // But if it exceeds the maximum, use the maximum
        if(
            swapChainSupportDetails.capabilities.maxImageCount > 0 &&
            minImageCount > swapChainSupportDetails.capabilities.maxImageCount
        )
        {
            minImageCount = swapChainSupportDetails.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainCreateInfo {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = minImageCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = swapChainExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndicesAsArray[] = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};
        if(queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndicesAsArray;
        }
        else
        {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0; // optional
            swapchainCreateInfo.pQueueFamilyIndices = nullptr; // optional
        }

        // Don't apply transformation to image, like rotation
        swapchainCreateInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
        // Don't blend the alpha channel with other windows in the window system
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        // Don't care about pixels behind another window, it is better for performance
        swapchainCreateInfo.clipped = VK_TRUE;
        // Don't create a new swap chain if it becomes invalid
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        // Create the swap chain
        VkResult result = vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, nullptr, &swapChain);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain");
        }

        // Get the swap chain images
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(vkDevice, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(vkDevice, swapChain, &imageCount, swapChainImages.data());
    }

    void recreateSwapChain()
    {
        // Handle minimization by pausing until it is not minimized
        int width {0};
        int height {0};
        glfwGetFramebufferSize(window, &width, &height);
        while(width == 0 || height == 0)
        {
            glfwWaitEvents();
            glfwGetFramebufferSize(window, &width, &height);
        }

        vkDeviceWaitIdle(vkDevice);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createColorResources();
        createDepthResources();
        createFramebuffers();
    }
    
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
    {
        VkImageViewCreateInfo imageViewCreateInfo {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        VkResult result = vkCreateImageView(vkDevice, &imageViewCreateInfo, nullptr, &imageView);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image view.");
        }
        return imageView;
    }

    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for(size_t i {0}; i < swapChainImages.size(); i++)
        {
            std::cout << "-- creating image view " << i << std::endl;
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    static std::vector<char> readFile(const std::string & filename)
    {
        // open file and seek the end, to find the file size
        std::ifstream file {filename, std::ios::ate | std::ios::binary};

        if(!file.is_open())
        {
            throw std::runtime_error("Failed to open file");
        }

        // Get the file size from the head position, which is at the end of the file
        size_t file_size {(size_t) file.tellg()};
        // Make a buffer to store the content of the file
        std::vector<char> buffer(file_size);
        // Go back to the beggining of the file
        file.seekg(0); 
        // Store the content of the file in the buffer
        file.read(buffer.data(), file_size);
        // Close the file, since we already have its content
        file.close();
        // Return the buffer
        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char> & shader_code)
    {
        //std::cout << "createShaderModule" << std::endl;
        //std::cout << "Shader code size = " << shader_code.size() << std::endl;

        VkShaderModuleCreateInfo shaderModuleCreateInfo {};
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.codeSize = shader_code.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(shader_code.data());

        VkShaderModule shaderModule;
        VkResult result = vkCreateShaderModule(vkDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module.");
        }
        return shaderModule;
    }

    void createRenderPass()
    {
        VkAttachmentDescription colorAttachmentDescription {};
        colorAttachmentDescription.format = swapChainImageFormat;
        colorAttachmentDescription.samples = msaaSamples;
        colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentReference {};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // For depth buffer
        VkAttachmentDescription depthAttachmentDescription {};
        depthAttachmentDescription.format = findDepthFormat();
        depthAttachmentDescription.samples = msaaSamples;
        depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentReference {};
        depthAttachmentReference.attachment = 1;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // For MSAA
        VkAttachmentDescription colorAttachmentResolve {};
        colorAttachmentResolve.format = swapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentResolveRef {};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        // Subpass
        VkSubpassDescription subpassDescription {};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentReference;
        subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
        subpassDescription.pResolveAttachments = &colorAttachmentResolveRef;

        // Subpass dependencies
        VkSubpassDependency subpassDependency {};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> attachments
        {
            colorAttachmentDescription,
            depthAttachmentDescription,
            colorAttachmentResolve
        };
        VkRenderPassCreateInfo renderPassCreateInfo {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassCreateInfo.pAttachments = attachments.data();
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &subpassDependency;

        VkResult result = vkCreateRenderPass(vkDevice, &renderPassCreateInfo, nullptr, &renderPass);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render pass.");
        }
    }

    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboLayoutBinding {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // optional

        VkDescriptorSetLayoutBinding samplerLayoutBinding {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings =
        {
            uboLayoutBinding,
            samplerLayoutBinding
        };
        
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetLayoutCreateInfo.pBindings = bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(vkDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor set layout.");
        }
    }
    
    void createGraphicsPipeline()
    {
        std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
        std::vector<char> fragShaderCode = readFile("shaders/frag.spv");
        std::cout << "vert shader code size: " << vertShaderCode.size() << " bytes" << std::endl;
        std::cout << "frag shader code size: " << fragShaderCode.size() << " bytes" << std::endl;

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertPipelineShaderStageCreateInfo {};
        vertPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertPipelineShaderStageCreateInfo.module = vertShaderModule;
        vertPipelineShaderStageCreateInfo.pName = "main"; // function to call

        VkPipelineShaderStageCreateInfo fragPipelineShaderStageCreateInfo {};
        fragPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragPipelineShaderStageCreateInfo.module = fragShaderModule;
        fragPipelineShaderStageCreateInfo.pName = "main"; // function to call

        VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] =
        {
            vertPipelineShaderStageCreateInfo,
            fragPipelineShaderStageCreateInfo
        };

        // Use dynamic states, so don't have to configure the viewport size and etc in the pipeline
        std::vector<VkDynamicState> dynamicStates = 
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        // Set up thegraphics pipeline to accept vertex data from Vertex struct
        VkVertexInputBindingDescription bindingDescription {Vertex::getBindingDescription()};
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions {Vertex::getAttributeDescriptions()};

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Configure pipeline to draw triangles
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        // Configure viewport
        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor {};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo {};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        //viewportStateCreateInfo.pViewports = &viewport; // uncomment with not using dynamic states
        viewportStateCreateInfo.scissorCount = 1;
        //viewportStateCreateInfo.pScissors = &scissor; // uncomment with not using dynamic states

        VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo {};
        rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationCreateInfo.lineWidth = 1.0f;
        rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationCreateInfo.depthBiasConstantFactor = 0.0f; // optional
        rasterizationCreateInfo.depthBiasClamp = 0.0f; // optional
        rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f; // optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleStateCreateInfo.rasterizationSamples = msaaSamples;
        multisampleStateCreateInfo.minSampleShading = 1.0f; // optional
        multisampleStateCreateInfo.pSampleMask = nullptr; // optional
        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // optional
        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState {};
        colorBlendAttachmentState.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = VK_FALSE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // optional
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // optional
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // optional
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // optional

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // optional
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
        for(size_t i {0}; i < 4; i++)
        {
            colorBlendStateCreateInfo.blendConstants[i] = 0.0f; // optional
        }

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0; // optional
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // optional

        VkResult result = vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout.");
        }

        // For depth buffer
        VkPipelineDepthStencilStateCreateInfo depthStencil {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // optional
        depthStencil.maxDepthBounds = 1.0f; // optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // optional
        depthStencil.back = {}; // optional

        // Create the graphics pipeline

        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.stageCount = 2;
        graphicsPipelineCreateInfo.pStages = shaderStageCreateInfos;
        graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        graphicsPipelineCreateInfo.layout = pipelineLayout;
        graphicsPipelineCreateInfo.renderPass = renderPass;
        graphicsPipelineCreateInfo.subpass = 0;
        graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // optional
        graphicsPipelineCreateInfo.basePipelineIndex = -1; // optional

        result = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create graphics pipeline.");
        }
        
        // cleanup
        vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
        vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    }

    void createFramebuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        // create a framebuffer for each image view
        for(size_t i {0}; i < swapChainFramebuffers.size(); i++)
        {
            // order is important here
            std::array<VkImageView, 3> attachments
            {
                colorImageView,
                depthImageView,
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferCreateInfo {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferCreateInfo.pAttachments = attachments.data();
            framebufferCreateInfo.width = swapChainExtent.width;
            framebufferCreateInfo.height = swapChainExtent.height;
            framebufferCreateInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(vkDevice, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]);
            if(result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create framebuffer.");
            }
        }
    }

    void createCommandPool()
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        VkResult result = vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool.");
        }
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        VkResult result = vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, commandBuffers.data());
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command buffers.");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = 0; // optional
        commandBufferBeginInfo.pInheritanceInfo = nullptr; // optional

        VkResult result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer.");
        }

        VkRenderPassBeginInfo renderPassBeginInfo {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues {};
        clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        // Bind vertex buffer
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        // Bind descriptor sets
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0,
            1,
            &descriptorSets[currentFrame],
            0,
            nullptr
        );

        // Bind index buffer
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Since we are using dynamic states, we have to set the viewport and scissor before drawing
        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor {};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Replaced vkCmdDraw with vkCmdDrawIndexed, which draws the vertices from their indices
        //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vertexIndices.size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
        result = vkEndCommandBuffer(commandBuffer);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer.");
        }
    }

    void updateUniformBuffer(uint32_t frame)
    {
        static auto startTime {std::chrono::high_resolution_clock::now()};
        auto currentTime {std::chrono::high_resolution_clock::now()};
        float time {std::chrono::duration<float, std::chrono::seconds::period>(currentTime-startTime).count()};

        UniformBufferObject ubo {};
        // rotation around Z-axis, proportional to time
        ubo.model = glm::rotate(
            glm::mat4(1.0f),
            time * glm::radians(90.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        ubo.view = glm::lookAt(
            glm::vec3(2.0f, 2.0f, 2.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        ubo.proj = glm::perspective(
            glm::radians(45.0f),
            swapChainExtent.width / (float) swapChainExtent.height,
            0.1f,
            10.0f
        );
        // Invert Y axis, because GLM was made for OpenGL
        ubo.proj[1][1] *= -1;

        // Copy ubo data to uniformBuffersMapped
        memcpy(uniformBuffersMapped[frame], &ubo, sizeof(ubo));
    }

    void drawFrame()
    {
        vkWaitForFences(vkDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        
        VkResult result = vkAcquireNextImageKHR(vkDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if(result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreateSwapChain();
            return;
        }
        else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            std::cout << result << std::endl;
            throw std::runtime_error("Failed to acquire swap chain image.");
        }

        // Only reset the fence if we are submitting work
        vkResetFences(vkDevice, 1, &inFlightFences[currentFrame]);
        
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        updateUniformBuffer(currentFrame);

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer.");
        }

        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // optional
        
        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain image.");
        }

        currentFrame++;
        if(currentFrame == MAX_FRAMES_IN_FLIGHT)
        {
            currentFrame = 0;
        }
    }

    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreCreateInfo {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkResult result1;
        VkResult result2;
        VkResult result3;

        for(size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            result1 = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]);
            result2 = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);
            result3 = vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &inFlightFences[i]);

            if(result1 != VK_SUCCESS || result2 != VK_SUCCESS || result3 != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create sync objects for a frame.");
            }
        }
    }

    void createBuffer(
        VkDeviceSize bufferSize,
        VkBufferUsageFlags usageFlags,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkBuffer & buffer,
        VkDeviceMemory & bufferMemory
    )
    {
        VkBufferCreateInfo bufferCreateInfo {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = bufferSize;
        bufferCreateInfo.usage = usageFlags;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(vkDevice, &bufferCreateInfo, nullptr, &buffer);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer.");
        }

        VkMemoryRequirements memoryRequirements {};
        vkGetBufferMemoryRequirements(vkDevice, buffer, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = 
            findMemoryType(
                memoryRequirements.memoryTypeBits,
                memoryPropertyFlags);

        result = vkAllocateMemory(vkDevice, &memoryAllocateInfo, nullptr, &bufferMemory);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate memory for buffer.");
        }

        vkBindBufferMemory(vkDevice, buffer, bufferMemory, 0);
    }

    VkCommandBuffer beginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        // ignoring result
        vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, &commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // ignoring result
        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        // ignoring result
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // ignoring results
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        vkFreeCommandBuffers(vkDevice, commandPool, 1, &commandBuffer);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion {};
        copyRegion.srcOffset = 0; // optional
        copyRegion.dstOffset = 0; // optional
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void createVertexBuffer()
    {
        VkDeviceSize bufferSize {sizeof(vertices[0])*vertices.size()}; // space to store all vertices

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VkBufferUsageFlags stagingUsageFlags {VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
        VkMemoryPropertyFlags stagingMemoryPropertyFlags
        {
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
        createBuffer(
            bufferSize,
            stagingUsageFlags,
            stagingMemoryPropertyFlags,
            stagingBuffer,
            stagingBufferMemory
        );

        // Send vertex data to staging buffer
        void * data;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        VkBufferUsageFlags vertexUsageFlags
        {
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        };
        VkMemoryPropertyFlags vertexMemoryPropertyFlags {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        createBuffer(bufferSize, vertexUsageFlags, vertexMemoryPropertyFlags, vertexBuffer, vertexBufferMemory);
        
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        // cleanup
        vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
        vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer()
    {
        VkDeviceSize bufferSize {sizeof(vertexIndices[0])*vertexIndices.size()};

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VkBufferUsageFlags stagingUsageFlags {VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
        VkMemoryPropertyFlags stagingMemoryPropertyFlags
        {
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };
        createBuffer(
            bufferSize,
            stagingUsageFlags,
            stagingMemoryPropertyFlags,
            stagingBuffer,
            stagingBufferMemory
        );

        // Send vertex index data to staging buffer
        void * data;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertexIndices.data(), (size_t) bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        VkBufferUsageFlags indexBufferUsageFlags
        {
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        };
        VkMemoryPropertyFlags indexBufferMemoryPropertyFlags {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        createBuffer(bufferSize, indexBufferUsageFlags, indexBufferMemoryPropertyFlags, indexBuffer, indexBufferMemory);
        
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        // cleanup
        vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
        vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memoryProperties);

        for(uint32_t i {0}; i < memoryProperties.memoryTypeCount; i++)
        {
            if(
                typeFilter & (1 << i) &&
                (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags
            )
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type.");
    }

    void createUniformBuffers()
    {
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
        
        const VkDeviceSize bufferSize {sizeof(UniformBufferObject)};
        const VkBufferUsageFlags usageFlags {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT};
        const VkMemoryPropertyFlags memoryPropertyFlags
        {
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        for(size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            createBuffer(bufferSize, usageFlags, memoryPropertyFlags, uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(vkDevice, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes {};
        descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {};
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
        descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
        descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkResult result = vkCreateDescriptorPool(vkDevice, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor pool.");
        }
    }

    void createDescriptorSets()
    {
        // make MAX_FRAMES_IN_FLIGHT copies of descriptorSetLayout, stored in a vector
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = std::vector(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VkResult result = vkAllocateDescriptorSets(vkDevice, &descriptorSetAllocateInfo, descriptorSets.data());
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate descriptor sets.");
        }

        for(size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo descriptorBufferInfo {};
            descriptorBufferInfo.buffer = uniformBuffers[i];
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo descriptorImageInfo {};
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorImageInfo.imageView = textureImageView;
            descriptorImageInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &descriptorBufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // optional
            descriptorWrites[0].pTexelBufferView = nullptr; // optional

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = nullptr;
            descriptorWrites[1].pImageInfo = &descriptorImageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr; // optional

            vkUpdateDescriptorSets(vkDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createImage(
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        VkSampleCountFlagBits samples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usageFlags,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkImage & image,
        VkDeviceMemory & imageMemory
    )
    {
        VkImageCreateInfo imageCreateInfo {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = usageFlags;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.samples = samples;
        imageCreateInfo.flags = 0; // optional

        VkResult result = vkCreateImage(vkDevice, &imageCreateInfo, nullptr, &image);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image.");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(vkDevice, image, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, memoryPropertyFlags);

        result = vkAllocateMemory(vkDevice, &memoryAllocateInfo, nullptr, &imageMemory);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to alloc image memory.");
        }

        vkBindImageMemory(vkDevice, image, imageMemory, 0);
    }

    void transitionImageLayout(
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t mipLevels
    )
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            throw std::invalid_argument("Unsupported layout transition.");
        }

        vkCmdPipelineBarrier(commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent =
        {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        endSingleTimeCommands(commandBuffer);
    }

    VkFormat findSupportedFormat(
        const std::vector<VkFormat> & candidateFormats,
        VkImageTiling imageTiling,
        VkFormatFeatureFlags formatFeatureFlags
    )
    {
        for(VkFormat format : candidateFormats)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, format, &formatProperties);

            if(
                imageTiling == VK_IMAGE_TILING_LINEAR &&
                (formatProperties.linearTilingFeatures & formatFeatureFlags) == formatFeatureFlags
            )
            {
                return format;
            }
            else if (
                imageTiling == VK_IMAGE_TILING_OPTIMAL &&
                (formatProperties.optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags
            )
            {
                return format;
            }
        }

        throw std::runtime_error("Failed to find supported format.");
    }
    
    bool hasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat findDepthFormat()
    {
        std::vector<VkFormat> candidateFormats
        {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        return findSupportedFormat(
            candidateFormats,
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }
    
    void createDepthResources()
    {
        VkFormat depthFormat = findDepthFormat();

        createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            1,
            msaaSamples,
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            depthImage,
            depthImageMemory
        );
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    void generateMipmaps(
        VkImage image,
        VkFormat imageFormat,
        uint32_t textureWidth,
        uint32_t textureHeight,
        uint32_t mipLevels
    )
    {
        // First check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, imageFormat, &formatProperties);
        if(!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            throw std::runtime_error("Texture image format doesn't support linear blitting.");
        }

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;
        
        int32_t mipWidth {(int32_t) textureWidth};
        int32_t mipHeight {(int32_t) textureHeight};
        for(uint32_t i {1}; i < mipLevels; i++)
        {
            // transition previous miplevel to optimal reading

            barrier.subresourceRange.baseMipLevel = i-1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier
            );

            // Record blit command

            VkImageBlit blit {};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i-1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] =
            {
                mipWidth > 1 ? mipWidth/2 : 1,
                mipHeight > 1 ? mipHeight/2 : 1,
                1
            };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(
                commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &blit,
                VK_FILTER_LINEAR
            );

            // transition mipLevel i-1 to shader read only optimal

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier
            );

            // Divide both mip dimensions by 2.
            // If they're different, one of them will be 0 if we keep dividing by 2,
            // so only divide until it is 1.
            if(mipWidth > 1)
            {
                mipWidth /= 2;
            }
            if(mipHeight > 1)
            {
                mipHeight /= 2;
            }
        }

        // transition last mipLevel to shader read only optimal

        uint32_t lastMipLevel = mipLevels-1;
        barrier.subresourceRange.baseMipLevel = lastMipLevel;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // didn't transitioned last mipLevel, so it is dst
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // never read from last mipLevel, so it is write
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void createTextureImage()
    {
        int textureWidth, textureHeight, textureChannels;
        // STBI_rgb_alpha forces to load with an alpha channel
        stbi_uc * pixels {stbi_load(TEXTURE_PATH.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha)};
        if(!pixels)
        {
            throw std::runtime_error("Failed to load texture image.");
        }
        
        VkDeviceSize imageSize = textureWidth*textureHeight*4; // 4 bytes per pixel
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

        // Create a staging buffer to receive the image data
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VkBufferUsageFlags usageFlags {VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
        VkMemoryPropertyFlags memoryPropertyFlags {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
        createBuffer(imageSize, usageFlags, memoryPropertyFlags, stagingBuffer, stagingBufferMemory);

        // Copy the image data to the staging buffer
        void * data;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(vkDevice, stagingBufferMemory);

        // cleanup
        stbi_image_free(pixels);

        createImage(
            textureWidth,
            textureHeight,
            mipLevels,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory
        );

        // prepare image to copy from staging buffer to image
        transitionImageLayout(
            textureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            mipLevels
        );

        copyBufferToImage(
            stagingBuffer,
            textureImage,
            static_cast<uint32_t>(textureWidth),
            static_cast<uint32_t>(textureHeight)
        );

        generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, textureWidth, textureHeight, mipLevels);

        // The transition to shader read only optimal will be made in generateMipmaps.
        /*
        transitionImageLayout(
            textureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            mipLevels
        );
        */

        // cleanup
        vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
        vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
    }

    void createTextureImageView()
    {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    }

    void createTextureSampler()
    {
        VkSamplerCreateInfo samplerCreateInfo {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(vkPhysicalDevice, &physicalDeviceProperties);

        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        samplerCreateInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;

        samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.mipLodBias = 0.0f; // optional
        samplerCreateInfo.minLod = 0.0f; // optional
        samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;

        VkResult result = vkCreateSampler(vkDevice, &samplerCreateInfo, nullptr, &textureSampler);
        if(result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture sampler.");
        }
    }

    void loadModel()
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;

        if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
        {
            std::cerr << "Failed to load model." << std::endl;
            throw std::runtime_error(err);
        }

        
        // Stores a unique vertex and its index.
        // It is used only to verify if a vertex is new and assign a index to new vertices.
        // New vertices and their indices are then stored in vertices and vertexIndices.
        std::unordered_map<Vertex, uint32_t> uniqueVertices;

        for(const tinyobj::shape_t & shape : shapes)
        {
            for(const tinyobj::index_t & index : shape.mesh.indices)
            {
                Vertex vertex {};

                // Initialize vertex
                vertex.pos = {
                    attrib.vertices[3*index.vertex_index],
                    attrib.vertices[3*index.vertex_index + 1],
                    attrib.vertices[3*index.vertex_index + 2]
                };

                vertex.textureCoord = {
                    attrib.texcoords[2*index.texcoord_index],
                    1.0f - attrib.texcoords[2*index.texcoord_index+1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};
            
                // Only put vertex in vertices vector if it is new
                if(uniqueVertices.count(vertex) == 0)
                {
                    // The index of the unique vertex will autoincrement
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());

                    vertices.push_back(vertex);
                }
                
                vertexIndices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    VkSampleCountFlagBits getMaxUsableSampleCount()
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(vkPhysicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags availableSampleCountFlags {
            physicalDeviceProperties.limits.framebufferColorSampleCounts &
            physicalDeviceProperties.limits.framebufferDepthSampleCounts
        };

        std::array<VkSampleCountFlagBits, 7> sampleCountFlagBitsDescending {
            VK_SAMPLE_COUNT_64_BIT,
            VK_SAMPLE_COUNT_32_BIT,
            VK_SAMPLE_COUNT_16_BIT,
            VK_SAMPLE_COUNT_8_BIT,
            VK_SAMPLE_COUNT_4_BIT,
            VK_SAMPLE_COUNT_2_BIT,
            VK_SAMPLE_COUNT_1_BIT
        };

        // Select a sample count flag. The last one, VK_SAMPLE_COUNT_1_BIT, should always be available,
        // so the function should return before finishing this for loop.
        for(size_t i {0}; i < sampleCountFlagBitsDescending.size(); i++)
        {
            if(availableSampleCountFlags & sampleCountFlagBitsDescending[i])
            {
                return sampleCountFlagBitsDescending[i];
            }
        }

        throw std::runtime_error("Error in getMaxUsableSampleCount: no sample count flag selected.");
    }

    void createColorResources()
    {
        VkFormat colorFormat {swapChainImageFormat};

        createImage(
            swapChainExtent.width,
            swapChainExtent.height,
            1,
            msaaSamples,
            colorFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            colorImage,
            colorImageMemory
        );

        colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    void mainLoop()
    {
        // Keep the window open
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(vkDevice);
    }

    void cleanupSwapChain()
    {
        // Destroy resources for MSAA
        vkFreeMemory(vkDevice, colorImageMemory, nullptr);
        vkDestroyImageView(vkDevice, colorImageView, nullptr);
        vkDestroyImage(vkDevice, colorImage, nullptr);

        // Destroy framebuffers
        for(VkFramebuffer & framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
        }
        
        // Destroy depth buffer
        vkFreeMemory(vkDevice, depthImageMemory, nullptr);
        vkDestroyImageView(vkDevice, depthImageView, nullptr);
        vkDestroyImage(vkDevice, depthImage, nullptr);

        // Destroy the swap chain image views
        for(VkImageView & imageView : swapChainImageViews)
        {
            vkDestroyImageView(vkDevice, imageView, nullptr);
        }

        // Destroy the swap chain
        vkDestroySwapchainKHR(vkDevice, swapChain, nullptr);
    }

    void cleanup()
    {
        cleanupSwapChain();

        // Destroy uniform buffer objects, free its memories
        for(size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(vkDevice, uniformBuffers[i], nullptr);
            vkFreeMemory(vkDevice, uniformBuffersMemory[i], nullptr);
        }

        // Destroy texture sampler
        vkDestroySampler(vkDevice, textureSampler, nullptr);

        // Destroy texture image view
        vkDestroyImageView(vkDevice, textureImageView, nullptr);

        // Destroy texture image
        vkDestroyImage(vkDevice, textureImage, nullptr);

        // Destroy texture image memory
        vkFreeMemory(vkDevice, textureImageMemory, nullptr);

        // Destroy index buffer
        vkDestroyBuffer(vkDevice, indexBuffer, nullptr);

        // Free index buffer memory
        vkFreeMemory(vkDevice, indexBufferMemory, nullptr);

        // Destroy the vertex buffer
        vkDestroyBuffer(vkDevice, vertexBuffer, nullptr);

        // Free vertex buffer memory
        vkFreeMemory(vkDevice, vertexBufferMemory, nullptr);

        // Destroy semaphores and fences
        for(size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(vkDevice, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(vkDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(vkDevice, inFlightFences[i], nullptr);
        }

        // Don't need to destroy the command buffer.
        // It is destroyed when the command pool is destroyed.

        // Destroy command pool
        vkDestroyCommandPool(vkDevice, commandPool, nullptr);

        // Destroy the graphics pipeline
        vkDestroyPipeline(vkDevice, pipeline, nullptr);

        // Destroy the pipeline layout
        vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);

        // Destroy descriptor pool
        vkDestroyDescriptorPool(vkDevice, descriptorPool, nullptr);

        // Destroy descriptor set layout
        vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout, nullptr);

        // Destroy the render pass
        vkDestroyRenderPass(vkDevice, renderPass, nullptr);

        // Don't need to cleanup the swap chain images

        // Don't need to cleanup the graphics queue.
        // it is destroyed when the (logical?) device is destroyed

        // Destroy the logical device
        vkDestroyDevice(vkDevice, nullptr);

        // Don't need to cleanup the physical device,
        // it is already destroyed together with the Vulkan instance

        // Destroy the window surface
        vkDestroySurfaceKHR(vkInstance, surface, nullptr);

        // Destroy the Vulkan instance
        // The nullptr refers to the callback allocator
        vkDestroyInstance(vkInstance, nullptr);

        // Destroy the GLFW window
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    Application app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
