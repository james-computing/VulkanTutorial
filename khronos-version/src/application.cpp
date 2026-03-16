#include "../include/application.hpp"

void Application::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Application::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void Application::cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // don't create an OpenGL context, since we're using Vulkan
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // don't allow resizing the window for the moment

    // Create a window.
    // The 4th parameter is to specify a monitor,
    // The 5th is for OpenGL.
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

std::vector<char const *> Application::getRequiredGLFWExtensions() const {
    // Get the required instance extensions from GLFW
    uint32_t glfwExtensionCount;
    char const ** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Replace glfwExtensions by a vector
    std::vector<char const *> requiredGLFWExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Also require the extension necessary for the message callback
    if(enableValidationLayers) {
        requiredGLFWExtensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    // Check if the required GLFW extensions are supported by the Vulkan implementation
    
    // try catch?
    std::vector<vk::ExtensionProperties> const extensionProperties {context.enumerateInstanceExtensionProperties()};

    // Print available extensions
    std::cout << "available extensions:\n";
    for (vk::ExtensionProperties const & extensionProperty : extensionProperties) {
        std::cout << '\t' << extensionProperty.extensionName << '\n';
    }

    // Find if there is a required GLFW extension which is none of the extension properties
    auto unsupportedIterator {
        std::ranges::find_if(
            requiredGLFWExtensions,
            [extensionProperties](char const * const & requiredGLFWExtension) -> bool {
                return std::ranges::none_of(
                extensionProperties,
                [requiredGLFWExtension](vk::ExtensionProperties const & extensionProperty) -> bool {
                    return strcmp(extensionProperty.extensionName, requiredGLFWExtension) == 0;
                });
            }
        )
    };

    if (unsupportedIterator != requiredGLFWExtensions.end()) {
        std::cerr << "Required GLFW extension not supported: " + std::string(*unsupportedIterator);
    }

    return requiredGLFWExtensions;
}

std::vector<char const *> Application::getRequiredValidationLayers() const {
    // Get the required validation layers
    std::vector<char const *> requiredValidationLayers;
    if (enableValidationLayers) {
        requiredValidationLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // try catch?
    std::vector<vk::LayerProperties> const layerProperties {context.enumerateInstanceLayerProperties()};

    // Find if there is a required validation layer that is none of the layer properties
    auto unsupportedLayerIterator {
        std::ranges::find_if(
            requiredValidationLayers,
            [&layerProperties] (char const * const &requiredValidationLayer) -> bool {
                return std::ranges::none_of(
                    layerProperties,
                    [requiredValidationLayer] (vk::LayerProperties const & layerProperty) -> bool {
                        return strcmp(layerProperty.layerName, requiredValidationLayer) == 0;
                    }
                );
            }
        )
    };

    if (unsupportedLayerIterator != requiredValidationLayers.end()) {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIterator));
    }

    return requiredValidationLayers;
}

void Application::createInstance() {
    std::vector<char const *> const requiredGLFWExtensions = getRequiredGLFWExtensions();
    std::vector<char const *> const requiredValidationLayers = getRequiredValidationLayers();

    vk::ApplicationInfo constexpr appInfo {
        .pApplicationName = "Application",
        .applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0), // VK_MAKE_VERSION is deprecated
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0), // VK_MAKE_VERSION is deprecated
        .apiVersion = vk::ApiVersion14
    };

    vk::InstanceCreateInfo const createInfo {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size()),
        .ppEnabledLayerNames = requiredValidationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredGLFWExtensions.size()),
        .ppEnabledExtensionNames = requiredGLFWExtensions.data()
    };

    // try catch?
    instance = vk::raii::Instance(context, createInfo);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Application::debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT        severity,
    vk::DebugUtilsMessageTypeFlagsEXT               type,
    vk::DebugUtilsMessengerCallbackDataEXT const *  pCallBackData,
    void *                                          pUserData
) {
    std::cerr << "validation layer: type " << vk::to_string(type) << " msg: " << pCallBackData->pMessage << std::endl;

    return vk::False;
}

void Application::setupDebugMessenger() {
    if (!enableValidationLayers) {
        return;
    }

    vk::DebugUtilsMessageSeverityFlagsEXT constexpr severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );

    vk::DebugUtilsMessageTypeFlagsEXT constexpr messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );

    vk::DebugUtilsMessengerCreateInfoEXT const debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback
    };

    // try catch?
    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

void Application::pickPhysicalDevice() {
    // try catch?
    std::vector<vk::raii::PhysicalDevice> const physicalDevices {instance.enumeratePhysicalDevices()};
    
    auto const deviceIterator {
        // Find if there is some suitable device
        std::ranges::find_if(
            physicalDevices,
            [this] (vk::raii::PhysicalDevice const & physicalDevice) -> bool {
                return isDeviceSuitable(physicalDevice);
            }
        )
    };

    if (deviceIterator == physicalDevices.end()) {
        throw std::runtime_error("Failed to find a suitable physical device");
    }

    // Pick the first suitable physical device found
    physicalDevice = *deviceIterator;
}

bool Application::isDeviceSuitable(vk::raii::PhysicalDevice const & physicalDevice) const {
    vk::PhysicalDeviceProperties const deviceProperties {physicalDevice.getProperties()};
    vk::PhysicalDeviceFeatures const deviceFeatures {physicalDevice.getFeatures()};
    std::vector<vk::QueueFamilyProperties> const queueFamilies {physicalDevice.getQueueFamilyProperties()};
    std::vector<char const *> const requiredDeviceExtensions({vk::KHRSwapchainExtensionName});

    bool const supportsVulkan1_3 {deviceProperties.apiVersion >= vk::ApiVersion13};

    bool const supportsGraphics {
        std::ranges::any_of(
            queueFamilies,
            [] (vk::QueueFamilyProperties const &qfp) -> bool {
                return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
            }
        )
    };

    // try catch?
    std::vector<vk::ExtensionProperties> const availableDeviceExtensions {physicalDevice.enumerateDeviceExtensionProperties()};

    bool const supportsAllRequiredExtensions {
        // All of the required device extensions are any of the available extensions
        std::ranges::all_of(
            requiredDeviceExtensions,
            [&availableDeviceExtensions] (char const * const & requiredDeviceExtension) -> bool {
                return std::ranges::any_of(
                    availableDeviceExtensions,
                    [requiredDeviceExtension] (vk::ExtensionProperties const & availableDeviceExtension) -> bool {
                        return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
                    }
                );
            }
        )
    };

    auto const features {
        // .template is to tell the compiler to use the method that comes from templates, avoiding ambiguity
        physicalDevice.template getFeatures2<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        >()
    };
    bool const supportsRequiredFeatures {
        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState
    };

    return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;

    /*
    if (
        deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu &&
        deviceFeatures.geometryShader &&
        supportsVulkan1_3
    ) {
        return true;
    }

    return false;*/
}

void Application::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> const queueFamilyProperties {
        physicalDevice.getQueueFamilyProperties()
    };

    // Find first queue with graphics support which is also capable of presenting to the window,
    // and store its index.
    bool foundSuitableQueue {false};
    uint32_t queueIndex {0};
    size_t const queueFamilyPropertiesSize {queueFamilyProperties.size()};
    for (; queueIndex < queueFamilyPropertiesSize; ++queueIndex) {
        bool supportsGraphics = (queueFamilyProperties[queueIndex].queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
        
        // try catch?
        bool supportsWindowPresentation = physicalDevice.getSurfaceSupportKHR(queueIndex, *surface);

        if (supportsGraphics && supportsWindowPresentation) {
            foundSuitableQueue = true;
            break;
        }
    }

    if (!foundSuitableQueue) {
        throw std::runtime_error("Failed to find suitable queue");
    }

    float constexpr queuePriority {0.5f};
    vk::DeviceQueueCreateInfo const deviceQueueCreateInfo {
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    //vk::PhysicalDeviceFeatures constexpr deviceFeatures;

    // Create a chain of featured structures.
    // Vulkan uses multiple features by chaining the features and then passing the first feature of the chain.
    // In C, the chain is constructed using the pNext property.
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > const featureChain {
        {},
        {.dynamicRendering = true},
        {.extendedDynamicState = true}
    };

    std::vector<char const *> const requiredDeviceExtensions {
        vk::KHRSwapchainExtensionName
    };

    vk::DeviceCreateInfo const deviceCreateInfo {
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data()
    };

    // try catch?
    device = vk::raii::Device(physicalDevice, deviceCreateInfo);

    queue = vk::raii::Queue(device, queueIndex, 0);
}

void Application::createSurface() {
    // C struct
    VkSurfaceKHR _surface;
    // C function call
    VkResult result = glfwCreateWindowSurface(*instance, window, nullptr, &_surface);

    if (result != VkResult::VK_SUCCESS) {
        std::cerr << "Failed to create window surface";
        return;
    }

    // Get a C++ surface from the C _surface
    surface = vk::raii::SurfaceKHR(instance, _surface);
}

vk::SurfaceFormatKHR Application::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const & availableFormats) const {
    auto const formatIterator{
        std::ranges::find_if(
            availableFormats,
            [] (vk::SurfaceFormatKHR const & availableFormat) -> bool {
                return availableFormat.format == vk::Format::eB8G8R8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            }
        )
    };

    if (formatIterator == availableFormats.end()) {
        return availableFormats[0];
    } else {
        return *formatIterator;
    }
}

vk::PresentModeKHR Application::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const & availablePresentModes) const {
    bool fifoAvailable {false};
    for (vk::PresentModeKHR const & presentMode : availablePresentModes) {
        switch (presentMode) {
            case vk::PresentModeKHR::eMailbox:
                return vk::PresentModeKHR::eMailbox;
            case vk::PresentModeKHR::eFifo:
                fifoAvailable = true;
                break;
        }
    }

    if (fifoAvailable) {
        return vk::PresentModeKHR::eFifo;
    }
    else {
        throw std::runtime_error("Neither eFifo or eMailbox present modes avaiable"); 
    }
}

vk::Extent2D Application::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const & capabilities) const {
    // If width != max, capabilities.currentExtent already have the correct Extent2D, just return it
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    // Otherwise, we can choose an extent.
    // Width and height must be between the minimum and maximum values allowed, we solve this by clamping.
    // The width and height must be in pixels, the appropriate values are obtained from the framebuffer size.
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return vk::Extent2D {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t Application::chooseSwapImageCount(vk::SurfaceCapabilitiesKHR const & surfaceCapabilities) const {
    // Pick at least 3 images, and at least the minimum + 1
    uint32_t minImageCount {std::max(3u, surfaceCapabilities.minImageCount + 1)};

    // Don't pass the maximum
    bool const thereIsAMax {surfaceCapabilities.maxImageCount > 0};
    if (thereIsAMax && surfaceCapabilities.maxImageCount < minImageCount) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }

    return minImageCount;
}

void Application::createSwapChain() {
    // Same from createLogicalDevice
    // try catch?
    vk::SurfaceCapabilitiesKHR const surfaceCapabilities {physicalDevice.getSurfaceCapabilitiesKHR(*surface)};
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    uint32_t const minImageCount {chooseSwapImageCount(surfaceCapabilities)};

    // Same from createLogicalDevice
    // try catch?
    std::vector<vk::SurfaceFormatKHR> const availableFormats {physicalDevice.getSurfaceFormatsKHR(*surface)};
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);

    // try catch?
    std::vector<vk::PresentModeKHR> const availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
    vk::PresentModeKHR const presentMode {chooseSwapPresentMode(availablePresentModes)};

    vk::SwapchainCreateInfoKHR const swapChainCreateInfo {
        .surface = *surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1, // because not a stereoscopic 3D application
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform, // don't apply any transformation
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = true
    };
    
    // try catch?
    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    // try catch?
    swapChainImages = swapChain.getImages();
}

void Application::createImageViews() {
    assert(swapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo {
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    for (vk::Image & image : swapChainImages) {
        imageViewCreateInfo.image = image;
        // vk::raii::CreateImageView is implicit? Compiler says the function doesn't exists
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
}