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
    pickPhysicalDevice();
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

std::vector<char const *> Application::getRequiredGLFWExtensions() {
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
    vk::ResultValue<std::vector<vk::ExtensionProperties>> const resultValueExtensionProperties {context.enumerateInstanceExtensionProperties()};
    if (!resultValueExtensionProperties.has_value()) {
        throw std::runtime_error("Failed to get extension properties in createInstance.");
    }

    std::vector<vk::ExtensionProperties> const extensionProperties {resultValueExtensionProperties.value};

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

std::vector<char const *> Application::getRequiredValidationLayers() {
    // Get the required validation layers
    std::vector<char const *> requiredValidationLayers;
    if (enableValidationLayers) {
        requiredValidationLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation
    vk::ResultValue<std::vector<vk::LayerProperties>> const resultValueLayerProperties {context.enumerateInstanceLayerProperties()};
    if (!resultValueLayerProperties.has_value()) {
        throw std::runtime_error("Failed to enumerate instance layer properties");
    }

    std::vector<vk::LayerProperties> const layerProperties {resultValueLayerProperties.value};

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

    vk::ResultValue<vk::raii::Instance> resultValueInstance {context.createInstance(createInfo, nullptr)};
    if (!resultValueInstance.has_value()) {
        std::cerr << "Failed to create Vulkan instance in Application::createInstance";
        return;
    }

    instance = std::move(resultValueInstance.value);
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

    vk::ResultValue<vk::raii::DebugUtilsMessengerEXT> resultValue {instance.createDebugUtilsMessengerEXT(
        debugUtilsMessengerCreateInfoEXT,
        nullptr
    )};

    if (!resultValue.has_value()) {
        std::cerr << "Failed to create debug messenger" << std::endl;
        return;
    }

    debugMessenger = std::move(resultValue.value);
}

void Application::pickPhysicalDevice() {
    vk::ResultValue<std::vector<vk::raii::PhysicalDevice>> const resultValue {instance.enumeratePhysicalDevices()};
    if (!resultValue.has_value()) {
        throw std::runtime_error("Failed to enumerate physical devices");
    }

    std::vector<vk::raii::PhysicalDevice> const physicalDevices {resultValue.value};
    
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

bool Application::isDeviceSuitable(vk::raii::PhysicalDevice const & physicalDevice) {
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

    //

    vk::ResultValue<std::vector<vk::ExtensionProperties>> const resultValueExtensionProperties {
        physicalDevice.enumerateDeviceExtensionProperties()
    };
    if (!resultValueExtensionProperties.has_value()) {
        std::cerr << "Failed to enumerate device extension properties";
        return false;
    }

    std::vector<vk::ExtensionProperties> const availableDeviceExtensions {resultValueExtensionProperties.value};

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
        bool supportsWindowPresentation = physicalDevice.getSurfaceSupportKHR(queueIndex, *surface).value;
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

    vk::ResultValue<vk::raii::Device> resultValueDevice {physicalDevice.createDevice(deviceCreateInfo)};
    if (!resultValueDevice.has_value()) {
        std::cerr << "Failed to create logical device";
        return;
    }

    device = std::move(resultValueDevice.value);

    queue = device.getQueue(queueIndex, 0);
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