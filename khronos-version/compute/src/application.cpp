#include "../include/application.hpp"

void Application::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Application::initVulkan() {
    std::cout << "Initializing Vulkan" << std::endl;
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
#ifndef COMPUTE
    createVertexBuffer();
#endif
    createCommandBuffers();
    createSyncObjects();

#ifdef COMPUTE
    createComputeResources();
#endif
    std::cout << "Vulkan initialized" << std::endl;
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
#ifdef COMPUTE
        updateTimeVariables();
#endif
    }
}

void Application::cleanup() {
    cleanupSwapChain();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // don't create an OpenGL context, since we're using Vulkan
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create a window.
    // The 4th parameter is to specify a monitor,
    // The 5th is for OpenGL.
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, frameBufferResizeCallback);
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
    std::cerr << "\nvalidation layer:\n" <<
                    "\ttype " << vk::to_string(type) << '\n' <<
                    "\tmsg: " << pCallBackData->pMessage << std::endl;

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
            vk::PhysicalDeviceVulkan11Features, // for shader module creation
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        >()
    };
    bool const supportsRequiredFeatures {
        
        features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&// for shader module creation
        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
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
    queueIndex = 0;
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
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
        vk::PhysicalDeviceVulkan11Features
    > const featureChain {
        {},
        {
            .synchronization2 = true, // sync objects
            .dynamicRendering = true
        },
        {.extendedDynamicState = true},
        {.shaderDrawParameters = true} // for shader module creation
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

void Application::createGraphicsPipeline() {
    std::vector<char> const shaderCode {readFile("shaders/slang.spv")};
    std::cout << "Shader code size = " << shaderCode.size() << " bytes" << std::endl;

    // The shader module is only needed during the pipeline creation,
    // so we can keep it as a local variable for this method.
    vk::raii::ShaderModule const shaderModule = createShaderModule(shaderCode);

    vk::PipelineShaderStageCreateInfo const vertShaderStageCreateInfo {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };

    vk::PipelineShaderStageCreateInfo const fragShaderStageCreateInfo {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"
    };

    vk::PipelineShaderStageCreateInfo const shaderStageCreateInfos[] {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

    std::vector<vk::DynamicState> const dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo const pipelineDynamicStateCreateInfo {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

#ifndef COMPUTE
    vk::VertexInputBindingDescription constexpr bindingDescription {Vertex::getBindingDescription()};
    std::array<vk::VertexInputAttributeDescription, 2> constexpr attributeDescriptions {Vertex::getAttributeDescriptions()};
#else
    vk::VertexInputBindingDescription constexpr bindingDescription {Particle::getBindingDescription()};
    std::array<vk::VertexInputAttributeDescription, 2> constexpr attributeDescriptions {Particle::getAttributeDescriptions()};
#endif
    vk::PipelineVertexInputStateCreateInfo const vertexInputCreateInfo {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo constexpr inputAssemblyCreateInfo {
        .topology = vk::PrimitiveTopology::ePointList // draw points
    };

    vk::Viewport const viewport {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(swapChainExtent.width),
        .height = static_cast<float>(swapChainExtent.height),
        .minDepth = 0,
        .maxDepth = 1
    };

    vk::Rect2D const rect2D {
        .offset = vk::Offset2D{0,0},
        .extent = swapChainExtent
    };

    vk::PipelineViewportStateCreateInfo constexpr pipelineViewportStateCreateInfo {
        .viewportCount = 1,
        .scissorCount = 1
    };

    vk::PipelineRasterizationStateCreateInfo constexpr pipelineRasterizationStateCreateInfo {
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo constexpr pipelineMultisampleStateCreateInfo {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState constexpr pipelineColorBlendAttachmentState {
        .blendEnable =      vk::False,
        .colorWriteMask =   vk::ColorComponentFlagBits::eR |
                            vk::ColorComponentFlagBits::eG |
                            vk::ColorComponentFlagBits::eB |
                            vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo const pipelineColorBlendStateCreateInfo {
        .logicOpEnable =    vk::False,
        .logicOp =          vk::LogicOp::eCopy,
        .attachmentCount =  1,
        .pAttachments =     &pipelineColorBlendAttachmentState
    };

    vk::PipelineLayoutCreateInfo constexpr pipelineLayoutCreateInfo {
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };

    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);

    vk::PipelineRenderingCreateInfo const pipelineRenderingCreateInfo {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainSurfaceFormat.format
    };

    vk::GraphicsPipelineCreateInfo const graphicsPipelineCreateInfo {
        .pNext =                &pipelineRenderingCreateInfo,
        .stageCount =           2,
        .pStages =              shaderStageCreateInfos,
        .pVertexInputState =    &vertexInputCreateInfo,
        .pInputAssemblyState =  &inputAssemblyCreateInfo,
        .pViewportState =       &pipelineViewportStateCreateInfo,
        .pRasterizationState =  &pipelineRasterizationStateCreateInfo,
        .pMultisampleState =    &pipelineMultisampleStateCreateInfo,
        .pColorBlendState =     &pipelineColorBlendStateCreateInfo,
        .pDynamicState =        &pipelineDynamicStateCreateInfo,
        .layout =               pipelineLayout,
        .renderPass =           nullptr, // because using dynamic rendering
        .basePipelineHandle =   VK_NULL_HANDLE, // optional
        .basePipelineIndex =    -1 // optional
    };

    // try catch?
    graphicsPipeline = vk::raii::Pipeline(device, nullptr, graphicsPipelineCreateInfo);
}

std::vector<char> Application::readFile(std::string const & filename) {
    // ate = start reading at the end of the file. This is used to get the file size.
    // binary is to read as binary, avoiding text transformations.
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file");
    }

    // Create a buffer with the file size
    size_t const fileSize {(size_t) file.tellg()};
    std::vector<char> buffer(fileSize);
    // Go to beggining of the file
    file.seekg(0);
    // Read the whole file to the buffer
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

[[nodiscard]] vk::raii::ShaderModule Application::createShaderModule(std::vector<char> const & code) const {
    // Size of type used for code, in bytes.
    // If the type is char, then typeSizeInBytes = 1.
    size_t constexpr typeSizeInBytes {sizeof(*code.data())};

    // The code must be passed as a pointer of type uint32_t
    uint32_t const * const codeReinterpreted = reinterpret_cast<uint32_t const *>(code.data());

    vk::ShaderModuleCreateInfo const shaderModuleCreateInfo {
        // code size is the size in bytes
        .codeSize = code.size() * typeSizeInBytes,
        .pCode = codeReinterpreted
    };

    vk::raii::ShaderModule shaderModule {vk::raii::ShaderModule(device, shaderModuleCreateInfo)};
    return shaderModule;
}

void Application::createCommandPool() {
    vk::CommandPoolCreateInfo const commandPoolCreateInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueIndex
    };

    commandPool = vk::raii::CommandPool(device, commandPoolCreateInfo);
}

void Application::createCommandBuffers() {
    vk::CommandBufferAllocateInfo const commandBufferAllocateInfo {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    // vk::raii::CommandBuffers inherits from std::vector<vk::raii:CommandBuffer>
    commandBuffers = vk::raii::CommandBuffers(device, commandBufferAllocateInfo);
}

void Application::transitionImageLayout(
    uint32_t imageIndex,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask
) const {
    // Use a barrier to change the image layout
    vk::ImageMemoryBarrier2 const barrier {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChainImages[imageIndex],
        .subresourceRange = vk::ImageSubresourceRange {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::DependencyInfo const dependencyInfo {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
}

void Application::recordCommandBuffer(uint32_t imageIndex) {
    vk::raii::CommandBuffer const & commandBuffer {commandBuffers[frameIndex]};

    commandBuffer.begin({});

    // Before start rendering, transition the swap chain image layout to COLOR_ATTACHMENT_OPTIMAL
    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eNone, // don't wait on previous operations
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    vk::ClearValue constexpr clearColor {vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)};

    vk::RenderingAttachmentInfo const renderingAttachmentInfo {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingInfo const renderingInfo {
        .renderArea = vk::Rect2D {
            .offset = {0, 0},
            .extent = swapChainExtent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &renderingAttachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

#ifndef COMPUTE
    commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
#else
    // the shader storage buffer is used by the graphics pipeline as the vertex buffer
    commandBuffer.bindVertexBuffers(0, {*(shaderStorageBuffers[frameIndex])}, {0});
#endif

    vk::Viewport const viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapChainExtent.width),
        .height = static_cast<float>(swapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    commandBuffer.setViewport(0, viewport);

    vk::Rect2D const scissor {
        .offset = vk::Offset2D(0, 0),
        .extent = swapChainExtent
    };

    commandBuffer.setScissor(0, scissor);

#ifndef COMPUTE
    commandBuffer.draw(vertices.size(), 1, 0, 0);
#else
    commandBuffer.draw(PARTICLE_COUNT, 1, 0, 0);
#endif

    commandBuffer.endRendering();

    // After rendering, transition the swapchain image to PRESENT_SRC
    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffer.end();
}

void Application::createSyncObjects() {
    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

    vk::FenceCreateInfo constexpr fenceCreateInfo {
        .flags = vk::FenceCreateFlagBits::eSignaled
    };

    size_t const numberOfImages {swapChainImages.size()};
    for (size_t i {0}; i < numberOfImages; ++i) {
        renderFinishedSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
    }

    for (size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
        inFlightFences.emplace_back(vk::raii::Fence(device, fenceCreateInfo));
    }
}

void Application::drawFrame() {

#ifdef COMPUTE
    vk::raii::Semaphore const & computeFinishedSemaphore {computeFinishedSemaphores[frameIndex]};

{   // Compute submission
    vk::raii::Fence const & computeInFlightFence {computeInFlightFences[frameIndex]};
    vk::raii::CommandBuffer const & computeCommandBuffer {computeCommandBuffers[frameIndex]};
    
    // Wait for fence.
    // Timeout is in nanoseconds.
    // UINT64_MAX is to wait as much as possible, but we're also using a while loop in case this isn't enough.
    vk::Result fenceResult;
    do {
        fenceResult = device.waitForFences(*computeInFlightFence, vk::True, UINT64_MAX);
    }
    while (fenceResult == vk::Result::eTimeout);

    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence");
    }

    updateComputeUniformBuffer(frameIndex);
    device.resetFences(*computeInFlightFence);
    computeCommandBuffer.reset();
    recordComputeCommandBuffer();

    vk::SubmitInfo const submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &*computeCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*computeFinishedSemaphore
    };
    queue.submit(submitInfo, *computeInFlightFence); 
}
#endif

{   // Graphics submission
    vk::raii::CommandBuffer & commandBuffer {commandBuffers[frameIndex]};
    vk::raii::Semaphore & presentCompleteSemaphore {presentCompleteSemaphores[frameIndex]};
    vk::raii::Fence & drawFence {inFlightFences[frameIndex]};

    // Wait for fence.
    // Timeout is in nanoseconds.
    // UINT64_MAX is to wait as much as possible, but we're also using a while loop in case this isn't enough.
    vk::Result fenceResult;
    do {
        fenceResult = device.waitForFences(*drawFence, vk::True, UINT64_MAX);
    }
    while (fenceResult == vk::Result::eTimeout);

    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence");
    }

    // Timeout is in nanoseconds. Use UINT64_MAX to effectivelly disable it.
    vk::ResultValue<uint32_t> const resultValueAcquireNextImage {swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore, {})};
    switch (resultValueAcquireNextImage.result) {
        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR: // resultValueAcquireNextImage.has_value() is giving false in this case, so must treat as error
            recreateSwapChain();
            return;
        case vk::Result::eSuccess:
            break;
        default:
            throw std::runtime_error("Failed to acquire next image");
    }
    if (!resultValueAcquireNextImage.has_value()) {
        throw std::runtime_error("resultValueAcquireNextImage.has_value() = false");
    }
    uint32_t const imageIndex {resultValueAcquireNextImage.value};

    commandBuffer.reset();
    recordCommandBuffer(imageIndex);

#ifndef COMPUTE
    uint32_t constexpr waitSemaphoreCount {1};
    vk::Semaphore waitSemaphores[] {
        *presentCompleteSemaphore
    };
    vk::PipelineStageFlags waitDestinationStageMasks[] {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
#else
    uint32_t constexpr waitSemaphoreCount {2};
    vk::Semaphore waitSemaphores[] {
        *presentCompleteSemaphore,
        *computeFinishedSemaphore
    };
    vk::PipelineStageFlags waitDestinationStageMasks[] {
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eVertexInput
    };
#endif
    vk::raii::Semaphore const & renderFinishedSemaphore {renderFinishedSemaphores[imageIndex]}; // imageIndex, not frameIndex

    vk::SubmitInfo const submitInfo {
        .waitSemaphoreCount = waitSemaphoreCount,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitDestinationStageMasks,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphore
    };

    device.resetFences(*drawFence);
    queue.submit(submitInfo, drawFence);

    vk::PresentInfoKHR const presentInfoKHR {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &*swapChain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr // optional
    };

    vk::Result const resultPresent {queue.presentKHR(presentInfoKHR)};
    if (resultPresent == vk::Result::eSuboptimalKHR || resultPresent == vk::Result::eErrorOutOfDateKHR || frameBufferResized) {
        recreateSwapChain();
        return;
    } else if (resultPresent != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present image");
    }

    ++frameIndex;
    if (frameIndex == MAX_FRAMES_IN_FLIGHT) {
        frameIndex = 0;
    }
}
}

void Application::cleanupSwapChain() {
    device.waitIdle();
    swapChainImageViews.clear();
    swapChain = nullptr;
}

void Application::recreateSwapChain() {
    // Handle window minimization by waiting for width and height to be non zero
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Now the window should not be minimized, with width and height non zero.
    // Proceed with swap chain recreation.

    cleanupSwapChain();
    createSwapChain();
    createImageViews();
}

void Application::frameBufferResizeCallback(GLFWwindow * window, int width, int height) {
    Application * const app {reinterpret_cast<Application *>(glfwGetWindowUserPointer(window))};
    app->frameBufferResized = true;
}

#ifndef COMPUTE
void Application::createVertexBuffer() {
    // Size of both staging and vertex buffers
    vk::DeviceSize const bufferSize {vertices.size() * sizeof(vertices[0])};

    // Create a staging buffer to transfer data from the host to the device
    vk::BufferUsageFlags constexpr stagingBufferUsage {vk::BufferUsageFlagBits::eTransferSrc};
    vk::MemoryPropertyFlags constexpr stagingBufferMemoryProperties {
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    };
    vk::raii::Buffer stagingBuffer {nullptr};
    vk::raii::DeviceMemory stagingBufferMemory {nullptr};
    createBuffer(
        bufferSize,
        stagingBufferUsage,
        stagingBufferMemoryProperties,
        stagingBuffer,
        stagingBufferMemory
    );

    // Copy the data from the vertices vector to the staging buffer memory
    void *data {stagingBufferMemory.mapMemory(0, bufferSize)};
    memcpy(data, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    // Create the vertex buffer
    vk::BufferUsageFlags constexpr vertexbufferUsage {vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst};
    vk::MemoryPropertyFlags constexpr vertexBufferMemoryProperties {vk::MemoryPropertyFlagBits::eDeviceLocal};
    createBuffer(
        bufferSize,
        vertexbufferUsage,
        vertexBufferMemoryProperties,
        vertexBuffer,
        vertexBufferMemory
    );

    // Copy data from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}
#endif

uint32_t Application::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties const memoryProperties {physicalDevice.getMemoryProperties()};

    for (uint32_t i {0}; i < memoryProperties.memoryTypeCount; ++i) {
        if (
            (typeFilter & (1 << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties
        ) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

void Application::createBuffer(
    vk::DeviceSize bufferSize,
    vk::BufferUsageFlags bufferUsage,
    vk::MemoryPropertyFlags memoryProperties,
    vk::raii::Buffer & buffer,
    vk::raii::DeviceMemory & bufferMemory
) {
    vk::BufferCreateInfo const bufferCreateInfo {
        .size = bufferSize,
        .usage = bufferUsage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    buffer = vk::raii::Buffer(device, bufferCreateInfo);

    vk::MemoryRequirements const memoryRequirements {buffer.getMemoryRequirements()};

    uint32_t const memoryTypeIndex {
        findMemoryType(
            memoryRequirements.memoryTypeBits,
            memoryProperties
        )
    };
    vk::MemoryAllocateInfo const memoryAllocateInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);

    vk::DeviceSize constexpr memoryOffset {0};
    buffer.bindMemory(*bufferMemory, memoryOffset);
}

void Application::copyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize bufferSize) const {
    vk::CommandBufferAllocateInfo const commandBufferAllocateInfo {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    vk::raii::CommandBuffer const commandCopyBuffer {std::move(device.allocateCommandBuffers(commandBufferAllocateInfo).front())};

    vk::CommandBufferBeginInfo constexpr commandBufferBeginInfo {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    commandCopyBuffer.begin(commandBufferBeginInfo);

    vk::BufferCopy const region {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = bufferSize
    };

    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, region);

    commandCopyBuffer.end();

    vk::SubmitInfo const submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandCopyBuffer
    };
    queue.submit(submitInfo, {});
    queue.waitIdle();
}

#ifdef COMPUTE
// For compute (bonus chapter) //////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::createComputeUniformBuffers() {
    // Adaptation of createUniformBuffers

    for (size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Create uniform buffer, allocate memory for it and bind it
        vk::DeviceSize constexpr bufferSize {sizeof(DeltaTime)};
        vk::BufferUsageFlags constexpr bufferUsage {vk::BufferUsageFlagBits::eUniformBuffer};
        vk::MemoryPropertyFlags constexpr memoryProperties {
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        };
        vk::raii::Buffer buffer {nullptr};
        vk::raii::DeviceMemory bufferMemory {nullptr};
        createBuffer(
            bufferSize,
            bufferUsage,
            memoryProperties,
            buffer,
            bufferMemory
        );
        computeUniformBuffers.emplace_back(std::move(buffer));
        computeUniformBuffersMemories.emplace_back(std::move(bufferMemory));
        
        // Map uniform buffer to a pointer, so we can transfer data from the pointer to the uniform buffer
        computeUniformBuffersMapped.emplace_back(computeUniformBuffersMemories[i].mapMemory(0, bufferSize));
    }
}

float clamp(float const value) {
    if (value < 0.0f) {
        return 0.0f;
    } else if (value > 1.0f) {
        return 1.0f;
    } else {
        return value;
    }
}

void Application::createShaderStorageBuffers() {
    
    // Make uniform distribution over interval [0,1]
    std::default_random_engine rndEngine((unsigned) time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initiliaze particles on a circle
    std::vector<Particle> particles(PARTICLE_COUNT);
    float const aspectRatio {((float) HEIGHT) / WIDTH};
    for (Particle & particle : particles) {
        // Make random radius and angle
        float const radius {0.25f * (1.0f + sqrtf(rndDist(rndEngine)))};
        float const theta {rndDist(rndEngine) * 2.0f * 3.14159265358979323846f};

        // Transfer to cartesian coordinates, taking into account the aspect ration
        float const x {radius * cosf(theta) * aspectRatio};
        float const y {radius * sinf(theta)};

        // Set particle
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.0001f;
        particle.color = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
    }

    // Create a staging buffer to upload data to the gpu
    vk::raii::Buffer stagingBuffer {nullptr};
    vk::raii::DeviceMemory stagingBufferMemory {nullptr};
    createBuffer(
        shaderStorageBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    // Copy particles data to staging buffer
    void * dataStaging {stagingBufferMemory.mapMemory(0, shaderStorageBufferSize)};
    memcpy(dataStaging, particles.data(), shaderStorageBufferSize);
    stagingBufferMemory.unmapMemory();

    // Create shader storage buffers and copy initial particle data to them.
    for (size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vk::raii::Buffer shaderStorageBufferTemp {nullptr};
        vk::raii::DeviceMemory shaderStorageBufferTempMemory {nullptr};
        createBuffer(
            shaderStorageBufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            shaderStorageBufferTemp,
            shaderStorageBufferTempMemory
        );
        copyBuffer(stagingBuffer, shaderStorageBufferTemp, shaderStorageBufferSize);
        shaderStorageBuffers.emplace_back(std::move(shaderStorageBufferTemp));
        shaderStorageBuffersMemories.emplace_back(std::move(shaderStorageBufferTempMemory));
    }
}

void Application::createComputeBuffers() {
    createComputeUniformBuffers();
    createShaderStorageBuffers();
}

void Application::createComputeDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding constexpr computeUniformBufferDescriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .pImmutableSamplers = nullptr
    };
    
    vk::DescriptorSetLayoutBinding constexpr storageBufferPreviousFrameDescriptorSetLayoutBinding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .pImmutableSamplers = nullptr
    };

    vk::DescriptorSetLayoutBinding constexpr storageBufferCurrentFrameDescriptorSetLayoutBinding {
        .binding = 2,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .pImmutableSamplers = nullptr
    };

    std::array<vk::DescriptorSetLayoutBinding, 3> constexpr descriptorSetLayoutBindings {
        computeUniformBufferDescriptorSetLayoutBinding,
        storageBufferPreviousFrameDescriptorSetLayoutBinding,
        storageBufferCurrentFrameDescriptorSetLayoutBinding
    };

    vk::DescriptorSetLayoutCreateInfo const descriptorSetLayoutCreateInfo {
        .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
        .pBindings = descriptorSetLayoutBindings.data()
    };

    computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);
}

void Application::createComputePipeline() {
    std::vector<char> const computeShaderCode {readFile("shaders/compute-slang.spv")};
    vk::raii::ShaderModule const computeShaderModule {createShaderModule(computeShaderCode)};

    vk::PipelineShaderStageCreateInfo const computeShaderStageCreateInfo {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = computeShaderModule,
        .pName = "compMain"
    };

    vk::PipelineLayoutCreateInfo const computePipelineLayoutCreateInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &*computeDescriptorSetLayout
    };

    computePipelineLayout = vk::raii::PipelineLayout(device, computePipelineLayoutCreateInfo);

    vk::ComputePipelineCreateInfo const computePipelineCreateInfo {
        .stage = computeShaderStageCreateInfo,
        .layout = *computePipelineLayout
    };

    computePipeline = vk::raii::Pipeline(device, nullptr, computePipelineCreateInfo);
}

void Application::createComputeDescriptorPool() {
    // Adaptation of createDescriptorPool

    vk::DescriptorPoolSize const computeUniformBufferDescriptorPoolSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT
    };

    vk::DescriptorPoolSize const shaderStorageBufferDescriptorPoolSize {
        .type = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 2 * MAX_FRAMES_IN_FLIGHT // 2 because of previous and current frames
    };

    std::array<vk::DescriptorPoolSize, 2> descriptorPoolSizes {
        computeUniformBufferDescriptorPoolSize,
        shaderStorageBufferDescriptorPoolSize
    };

    vk::DescriptorPoolCreateInfo const descriptorPoolCreateInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size()),
        .pPoolSizes = descriptorPoolSizes.data()
    };

    computeDescriptorPool = vk::raii::DescriptorPool(device, descriptorPoolCreateInfo);
}

void Application::createComputeDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> const descriptorSetLayouts(MAX_FRAMES_IN_FLIGHT, *computeDescriptorSetLayout);

    vk::DescriptorSetAllocateInfo const descriptorSetAllocateInfo {
        .descriptorPool = *computeDescriptorPool,
        .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = descriptorSetLayouts.data()
    };
    
    computeDescriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);

    for (size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vk::DescriptorBufferInfo const computeUniformBufferInfo {
            .buffer = *(computeUniformBuffers[i]),
            .offset = 0,
            .range = sizeof(DeltaTime)
        };

        vk::DescriptorBufferInfo const shaderStorageBufferPreviousFrameInfo {
            .buffer = *(shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT]),
            .offset = 0,
            .range = shaderStorageBufferSize
        };

        vk::DescriptorBufferInfo const shaderStorageBufferCurrentFrameInfo {
            .buffer = *(shaderStorageBuffers[i]),
            .offset = 0,
            .range = shaderStorageBufferSize
        };

        vk::WriteDescriptorSet const computeUniformBufferWriteDescriptorSet {
            .dstSet = *(computeDescriptorSets[i]),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &computeUniformBufferInfo,
            .pTexelBufferView = nullptr
        };

        vk::WriteDescriptorSet const shaderStorageBufferPreviousFrameWriteDescriptorSet {
            .dstSet = *(computeDescriptorSets[i]),
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &shaderStorageBufferPreviousFrameInfo,
            .pTexelBufferView = nullptr
        };

        vk::WriteDescriptorSet const shaderStorageBufferCurrentFrameWriteDescriptorSet {
            .dstSet = *(computeDescriptorSets[i]),
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pImageInfo = nullptr,
            .pBufferInfo = &shaderStorageBufferCurrentFrameInfo,
            .pTexelBufferView = nullptr
        };

        std::array<vk::WriteDescriptorSet, 3> const writeDescriptorSets {
            computeUniformBufferWriteDescriptorSet,
            shaderStorageBufferPreviousFrameWriteDescriptorSet,
            shaderStorageBufferCurrentFrameWriteDescriptorSet
        };

        device.updateDescriptorSets(writeDescriptorSets, {});
    }
}

void Application::createComputeCommandBuffers() {
    vk::CommandBufferAllocateInfo const commandBufferAllocateInfo {
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    computeCommandBuffers = vk::raii::CommandBuffers(device, commandBufferAllocateInfo);
}

void Application::createComputeSyncObjects() {
    vk::FenceCreateInfo constexpr fenceCreateInfo {
        .flags = vk::FenceCreateFlagBits::eSignaled
    };

    vk::SemaphoreCreateInfo constexpr semaphoreCreateInfo {};

    for (size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        computeInFlightFences.emplace_back(vk::raii::Fence(device, fenceCreateInfo));
        computeFinishedSemaphores.emplace_back(vk::raii::Semaphore(device, semaphoreCreateInfo));
    }
}

void Application::createComputeResources() {
    createComputeDescriptorSetLayout();
    createComputePipeline();
    createComputeBuffers();
    createComputeDescriptorPool();
    createComputeDescriptorSets();
    createComputeCommandBuffers();
    createComputeSyncObjects();
}

void Application::recordComputeCommandBuffer() {
    vk::raii::CommandBuffer const & commandBuffer {computeCommandBuffers[frameIndex]};
    commandBuffer.begin({});

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *computePipelineLayout,
        0, // first descriptor set
        {computeDescriptorSets[frameIndex]},
        {}
    );
    commandBuffer.dispatch(PARTICLE_COUNT / NUM_THREADS, 1, 1);
    commandBuffer.end();
}

void Application::updateComputeUniformBuffer(uint32_t currentImage) {
    DeltaTime const ubo {
        .deltaTime = static_cast<float>(lastFrameTime) * 2.0f
    };
    
    memcpy(computeUniformBuffersMapped[currentImage], &ubo, sizeof(DeltaTime));
}

void Application::updateTimeVariables() {
    // We want to animate the particle system using the last frames time to get smooth, frame-rate independent animation
    double currentTime = glfwGetTime();
    lastFrameTime      = (currentTime - lastTime) * 1000.0;
    lastTime           = currentTime;
}

#endif