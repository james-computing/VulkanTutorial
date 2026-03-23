#include "../include/application.hpp"

// Include here to avoid multiple implementation.
// STB is for loading the texture image.
#define STB_IMAGE_IMPLEMENTATION
#include "../original-version/Rendering/libraries/stb/stb_image.h"

// Tiny obj loader is for loading the 3d model.
#define TINYOBJLOADER_IMPLEMENTATION
#include "../original-version/Rendering/libraries/tinyobjloader/tiny_obj_loader.h"

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
    msaaSamples = getMaxUsableSampleCount();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createColorResources();
    createDepthResources();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
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
    if (type >= vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation) {
        throw std::runtime_error("Vulkan error!");
    }
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
        
        features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters && // for shader module creation
        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState &&
        features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy // for texture sampler
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
        {.features = {.samplerAnisotropy = true }},
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

    // Could use createImageView in this for loop, but won't, because it could be creating multiple
    // copiies of imageViewCreateInfo unnecessarily.
    for (vk::Image & image : swapChainImages) {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(vk::raii::ImageView(device, imageViewCreateInfo));
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


    vk::VertexInputBindingDescription constexpr bindingDescription {Vertex::getBindingDescription()};
    std::array<vk::VertexInputAttributeDescription, 3> constexpr attributeDescriptions {Vertex::getAttributeDescriptions()};
    vk::PipelineVertexInputStateCreateInfo const vertexInputCreateInfo {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo constexpr inputAssemblyCreateInfo {
        .topology = vk::PrimitiveTopology::eTriangleList // triangle from every 3 vertices, without reuse.
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
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo const pipelineMultisampleStateCreateInfo {
        .rasterizationSamples = msaaSamples,
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

    vk::PipelineLayoutCreateInfo const pipelineLayoutCreateInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptorSetLayout,
        .pushConstantRangeCount = 0
    };

    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);

    // Maybe the depth resources should be created before the graphics pipeline?
    depthFormat = findDepthFormat();

    vk::PipelineRenderingCreateInfo const pipelineRenderingCreateInfo {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
        .depthAttachmentFormat = depthFormat
    };

    vk::PipelineDepthStencilStateCreateInfo constexpr depthStencilStateCreateInfo {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False
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
        .pDepthStencilState =   &depthStencilStateCreateInfo,
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
    vk::Image const & image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask,
    vk::ImageAspectFlags imageAspectFlags
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
        .image = image,
        .subresourceRange = vk::ImageSubresourceRange {
            .aspectMask = imageAspectFlags,
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
        swapChainImages[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eNone, // don't wait on previous operations
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
    );

    // Transition multisampled color image to eColorAttachmentOptimal
    transitionImageLayout(
        *colorImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
    );

    // Is it necessary to make this transition for every frame? There is a single transition for the depth buffer.
    transitionImageLayout(
        *depthImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth
    );

    vk::ClearValue constexpr clearColor {vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)}; // black

    // MSAA with resolve
    vk::RenderingAttachmentInfo const colorAttachmentInfo {
        .imageView = *colorImageView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = swapChainImageViews[imageIndex],
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::ClearValue constexpr clearDepth {vk::ClearDepthStencilValue(1.0f, 0)}; // 1.0 = far view plane

    vk::RenderingAttachmentInfo const depthAttachmentInfo {
        .imageView = depthImageView,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth
    };

    vk::RenderingInfo const renderingInfo {
        .renderArea = vk::Rect2D {
            .offset = {0, 0},
            .extent = swapChainExtent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

    commandBuffer.bindVertexBuffers(0, *vertexBuffer, {0});
    
    commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint32);

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

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *(descriptorSets[frameIndex]), nullptr);

    commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);

    commandBuffer.endRendering();

    // After rendering, transition the swapchain image to PRESENT_SRC
    transitionImageLayout(
        swapChainImages[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::ImageAspectFlagBits::eColor
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
    vk::raii::CommandBuffer & commandBuffer {commandBuffers[frameIndex]};
    vk::raii::Semaphore & presentCompleteSemaphore {presentCompleteSemaphores[frameIndex]};
    vk::raii::Fence & drawFence {inFlightFences[frameIndex]};

    // Timeout is in nanoseconds. Use UINT64_MAX to effectivelly disable it.
    vk::Result const fenceResult {device.waitForFences(*drawFence, vk::True, UINT64_MAX)};
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

    vk::raii::Semaphore const & renderFinishedSemaphore {renderFinishedSemaphores[imageIndex]}; // imageIndex, not frameIndex
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    vk::SubmitInfo const submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphore
    };

    updateUniformBuffer(frameIndex);

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
    createColorResources();
    createDepthResources();
}

void Application::frameBufferResizeCallback(GLFWwindow * window, int width, int height) {
    Application * const app {reinterpret_cast<Application *>(glfwGetWindowUserPointer(window))};
    app->frameBufferResized = true;
}

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
    void * data {stagingBufferMemory.mapMemory(0, bufferSize)};
    memcpy(data, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();
    data = nullptr;

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

void Application::copyBuffer(vk::raii::Buffer const & srcBuffer, vk::raii::Buffer const & dstBuffer, vk::DeviceSize bufferSize) const {
    vk::raii::CommandBuffer commandCopyBuffer {nullptr};
    beginSingleTimeCommands(commandCopyBuffer);

    vk::BufferCopy const region {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = bufferSize
    };

    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, region);

    endSingleTimeCommands(commandCopyBuffer);
}

void Application::createIndexBuffer() {
    // Size of both staging and vertex buffers
    vk::DeviceSize const bufferSize {indices.size() * sizeof(indices[0])};

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

    // Copy the data from the indices vector to the staging buffer memory
    void * data {stagingBufferMemory.mapMemory(0, bufferSize)};
    memcpy(data, indices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();
    data = nullptr;

    // Create the vertex buffer
    vk::BufferUsageFlags constexpr indexbufferUsage {vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst};
    vk::MemoryPropertyFlags constexpr indexBufferMemoryProperties {vk::MemoryPropertyFlagBits::eDeviceLocal};
    createBuffer(
        bufferSize,
        indexbufferUsage,
        indexBufferMemoryProperties,
        indexBuffer,
        indexBufferMemory
    );

    // Copy data from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void Application::createDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding constexpr uboDescriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr
    };

    vk::DescriptorSetLayoutBinding constexpr combinedImageSamplerDescriptorSetLayoutBinding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
    };

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings {
        uboDescriptorSetLayoutBinding, 
        combinedImageSamplerDescriptorSetLayoutBinding
    };

    vk::DescriptorSetLayoutCreateInfo const descriptorSetLayoutCreateInfo {
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, descriptorSetLayoutCreateInfo);
}

void Application::createUniformBuffers() {
    for (size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Create uniform buffer, allocate memory for it and bind it
        vk::DeviceSize constexpr bufferSize {sizeof(UniformBufferObject)};
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
        uniformBuffers.emplace_back(std::move(buffer));
        uniformBuffersMemories.emplace_back(std::move(bufferMemory));
        
        // Map uniform buffer to a pointer, so we can transfer data from the pointer to the uniform buffer
        uniformBuffersMapped.emplace_back(uniformBuffersMemories[i].mapMemory(0, bufferSize));
    }
}

void Application::updateUniformBuffer(uint32_t currentImage) {
    // Get the start time from the first call to this function.
    // Later calls won't update the start time.
    static auto const startTime {std::chrono::high_resolution_clock::now()};

    // Compute the time elapsed from start time to now. Elapsed time will parameterize the rotation.
    auto const currenTime {std::chrono::high_resolution_clock::now()};
    float const elapsedTime {std::chrono::duration<float, std::chrono::seconds::period>(currenTime - startTime).count()};

    // Update the uniform buffer
    UniformBufferObject ubo;

    glm::vec3 constexpr up {glm::vec3(0.0f, 0.0f, 1.0f)};

    // Rotate model around the z axis, according to the elapsed time.
    glm::mat4 constexpr identity {glm::mat4(1.0f)};
    ubo.model = glm::rotate(identity, elapsedTime * glm::radians(90.0f), up);

    // View the model from a 45° angle
    glm::vec3 constexpr eye {glm::vec3(2.0f, 2.0f, 2.0f)};
    glm::vec3 constexpr center {glm::vec3(0.0f, 0.0f, 0.0f)};
    ubo.view = glm::lookAt(eye, center, up);

    // Perspective projection
    float const aspectRatio {static_cast<float>(swapChainExtent.width)/static_cast<float>(swapChainExtent.height)};
    float constexpr near {0.1f};
    float constexpr far {10.f};
    ubo.proj = glm::perspective(glm::radians(45.0f), aspectRatio, near, far);
    // GLM was made for OpenGL. For Vulkan we need to flip the sign of the Y scaling factor.
    ubo.proj[1][1] *= -1;

    // Copy the ubo to the corresponding uniform buffer memory.
    // It would be more efficient to use push constants.
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Application::createDescriptorPool() {
    vk::DescriptorPoolSize const uniformBufferDescriptorPoolSize {
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT
    };

    vk::DescriptorPoolSize const combinedImageSamplerDescriptorPoolSize {
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT
    };

    std::array<vk::DescriptorPoolSize, 2> descriptorPoolSizes {
        uniformBufferDescriptorPoolSize,
        combinedImageSamplerDescriptorPoolSize
    };

    vk::DescriptorPoolCreateInfo const descriptorPoolCreateInfo {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size()),
        .pPoolSizes = descriptorPoolSizes.data()
    };

    descriptorPool = vk::raii::DescriptorPool(device, descriptorPoolCreateInfo);
}

void Application::createDescriptorSets() {
    // Vector with MAX_FRAMES_IN_FLIGHT copies of *descriptorSetLayout.
    // It is needed because descriptorSetAllocateInfo receives an array of layouts.
    std::vector<vk::DescriptorSetLayout> const descriptorSetLayouts {std::vector(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout)};

    // Allocate descriptor sets
    vk::DescriptorSetAllocateInfo const descriptorSetAllocateInfo {
        .descriptorPool = descriptorPool,
        .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = descriptorSetLayouts.data()
    };

    descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);

    // Configure descriptor sets.
    // Maybe could build an array of vk::WriteDescriptorSet and call device.updateDescriptorSets once.
    for (size_t i {0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vk::DescriptorBufferInfo const descriptorBufferInfo {
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };

        vk::WriteDescriptorSet const uniformBufferWriteDescriptorSet {
            .dstSet = descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &descriptorBufferInfo
        };

        vk::DescriptorImageInfo const descriptorImageInfo {
            .sampler = textureSampler,
            .imageView = textureImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet const combinedImageSamplerWriteDescriptorSet {
            .dstSet = descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &descriptorImageInfo
        };

        std::array<vk::WriteDescriptorSet, 2> writeDescriptorSets {
            uniformBufferWriteDescriptorSet,
            combinedImageSamplerWriteDescriptorSet
        };

        device.updateDescriptorSets(writeDescriptorSets, {});
    }
}

void Application::createTextureImage() {
    int textureWidth, textureHeight, textureChannels;
    stbi_uc * pixels {stbi_load(texturePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha)};
    if (!pixels) {
        throw std::runtime_error("Failed to load texture.");
    }
    
    vk::DeviceSize const imageSize {(vk::DeviceSize) (textureWidth * textureHeight * 4)};
    // sum 1 for the original image to have a mip level
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

    // Create a staging buffer to receive the image
    vk::raii::Buffer stagingBuffer {nullptr};
    vk::raii::DeviceMemory stagingBufferMemory {nullptr};
    vk::BufferUsageFlags constexpr stagingBufferUsageFlags {vk::BufferUsageFlagBits::eTransferSrc};
    vk::MemoryPropertyFlags constexpr stagingBufferMemoryProperties {
        // Memory visible to host and available immediately to the device
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    };
    createBuffer(
        imageSize,
        stagingBufferUsageFlags,
        stagingBufferMemoryProperties,
        stagingBuffer,
        stagingBufferMemory
    );

    // Transfer the image to the staging buffer
    void * data {stagingBufferMemory.mapMemory(0, imageSize)};
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();
    data = nullptr;

    // cleanup
    stbi_image_free(pixels);

    vk::ImageTiling constexpr imageTiling {vk::ImageTiling::eOptimal};
    vk::ImageUsageFlags constexpr imageUsage {
        vk::ImageUsageFlagBits::eTransferSrc | // for mip map creation
        vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eSampled
    };
    vk::MemoryPropertyFlags constexpr imageMemoryProperties {vk::MemoryPropertyFlagBits::eDeviceLocal};
    createImage(
        static_cast<uint32_t>(textureWidth),
        static_cast<uint32_t>(textureHeight),
        mipLevels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        imageTiling,
        imageUsage,
        imageMemoryProperties,
        textureImage,
        textureImageMemory
    );
    
    // Transition image layout to receive texture
    transitionTextureImageLayout(
        textureImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        mipLevels
    );
    // Copy texture from staging buffer to image
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
    // Transition image layout to be read from shader.
    // This transition will be done in generateMipmaps.
    /*
    transitionTextureImageLayout(
        textureImage,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        mipLevels
    );
    */

    generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, textureWidth, textureHeight, mipLevels);
}

void Application::createImage(
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    vk::SampleCountFlagBits numSamples,
    vk::Format imageFormat,
    vk::ImageTiling imageTiling,
    vk::ImageUsageFlags imageUsage,
    vk::MemoryPropertyFlags imageMemoryProperties,
    vk::raii::Image & image,
    vk::raii::DeviceMemory & imageMemory
) {
    vk::Extent3D const extent {
        .width = width,
        .height = height,
        .depth = 1
    };
    vk::ImageCreateInfo const imageCreateInfo {
        .imageType = vk::ImageType::e2D,
        .format = imageFormat,
        .extent = extent,
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = numSamples,
        .tiling = imageTiling,
        .usage = imageUsage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    image = vk::raii::Image(device, imageCreateInfo);

    // Allocate memory for the image
    vk::MemoryRequirements const memoryRequirements {image.getMemoryRequirements()};
    vk::MemoryAllocateInfo const memoryAllocateInfo {
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, imageMemoryProperties)
    };
    imageMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    // Bind the memory
    image.bindMemory(imageMemory, 0);
}

void Application::beginSingleTimeCommands(vk::raii::CommandBuffer & commandBuffer) const {
    vk::CommandBufferAllocateInfo const commandBufferAllocateInfo {
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    commandBuffer = std::move(device.allocateCommandBuffers(commandBufferAllocateInfo).front());

    vk::CommandBufferBeginInfo constexpr commandBufferBeginInfo {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    commandBuffer.begin(commandBufferBeginInfo);
}

void Application::endSingleTimeCommands(vk::raii::CommandBuffer const & commandBuffer) const {
    commandBuffer.end();

    vk::SubmitInfo const submitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer
    };
    queue.submit(submitInfo, {});
    queue.waitIdle();
}

void Application::transitionTextureImageLayout(
    vk::raii::Image const & image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    uint32_t mipLevels
) const {
    vk::raii::CommandBuffer commandBuffer {nullptr};
    beginSingleTimeCommands(commandBuffer);

    vk::ImageMemoryBarrier barrier {
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = vk::ImageSubresourceRange {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("Unsupported image layout transition");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);

    endSingleTimeCommands(commandBuffer);
}

void Application::copyBufferToImage(
    vk::raii::Buffer const & buffer,
    vk::raii::Image const & image,
    uint32_t width,
    uint32_t height
) const {
    vk::raii::CommandBuffer commandBuffer {nullptr};
    beginSingleTimeCommands(commandBuffer);


    vk::ImageSubresourceLayers constexpr imageSubresource {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1
    };
    vk::Offset3D constexpr offset3D {
        .x = 0,
        .y = 0,
        .z = 0
    };
    vk::BufferImageCopy const region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = imageSubresource,
        .imageOffset = offset3D, 
        .imageExtent = vk::Extent3D {
            .width = width,
            .height = height,
            .depth = 1
        }
    };

    commandBuffer.copyBufferToImage(*buffer, *image, vk::ImageLayout::eTransferDstOptimal, region);

    endSingleTimeCommands(commandBuffer);
}

vk::raii::ImageView Application::createImageView(
    vk::raii::Image const & image,
    vk::Format format,
    vk::ImageAspectFlags aspectFlags,
    uint32_t mipLevels
) const {
    vk::ImageViewCreateInfo const imageViewCreateInfo {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = vk::ImageSubresourceRange {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    return vk::raii::ImageView(device, imageViewCreateInfo);
}

void Application::createTextureImageView() {
    textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);
}

void Application::createTextureSampler() {
    vk::PhysicalDeviceProperties physicalDeviceProperties {physicalDevice.getProperties()};

    vk::SamplerCreateInfo const samplerCreateInfo {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = vk::LodClampNone,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False
    };

    textureSampler = vk::raii::Sampler(device, samplerCreateInfo);
}

vk::Format Application::findSupportedFormat(
    std::vector<vk::Format> const & candidateFormats,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
) const {
    switch (tiling) {
        case vk::ImageTiling::eLinear:
            for (vk::Format const & format : candidateFormats) {
                vk::FormatProperties const props {physicalDevice.getFormatProperties(format)};
                if ((props.linearTilingFeatures & features) == features) {
                    return format;
                }
            }
            break;
        case vk::ImageTiling::eOptimal:
            for (vk::Format const & format : candidateFormats) {
                vk::FormatProperties const props {physicalDevice.getFormatProperties(format)};
                if ((props.optimalTilingFeatures & features) == features) {
                    return format;
                }
            }
            break;
    }
    throw std::runtime_error("Failed to find supported format");
}

vk::Format Application::findDepthFormat() const {
    std::vector<vk::Format> const candidateFormats {
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint
    };

    return findSupportedFormat(candidateFormats, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool Application::hasStencilComponent(vk::Format format) const {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

void Application::createDepthResources() {
    // Initialized in pipeline creation
    //depthFormat = findDepthFormat();

    // Create depth image, allocate memory for it and bind it
    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        1,
        msaaSamples,
        depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        depthImage,
        depthImageMemory
    );

    // Create depth image view
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
}

void Application::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str())) {
        std::cerr << "Failed to load model" << std::endl;
        throw std::runtime_error(warn + err);
    }

    size_t triple_vertex_index;
    size_t double_texture_index;
    // Make a map to store a vertex and the index attribute to it in its first appearance
    std::unordered_map<Vertex, uint32_t> uniqueVertices {};
    uint32_t newVertexIndex;
    for (tinyobj::shape_t const & shape : shapes) {
        for (auto const & index : shape.mesh.indices) {
            Vertex vertex;

            triple_vertex_index = 3 * index.vertex_index;
            vertex.position = {
                attrib.vertices[triple_vertex_index],
                attrib.vertices[triple_vertex_index + 1],
                attrib.vertices[triple_vertex_index + 2]
            };

            double_texture_index = 2 * index.texcoord_index;
            vertex.textureCoord = {
                attrib.texcoords[double_texture_index],
                1.0f - attrib.texcoords[double_texture_index + 1]
            };

            // Do we need a color?
            vertex.color = {1.0f, 1.0f, 1.0f};

            // If the vertex is new, store it in uniqueVertices and give it an index
            if (uniqueVertices.count(vertex) == 0) {
                // Create an index
                newVertexIndex = static_cast<uint32_t>(vertices.size());
                // Store the vertex and its index in the map
                uniqueVertices[vertex] = newVertexIndex;
                // Store the vertex is the vertices vector
                vertices.emplace_back(vertex);
            }
            
            // Store the index of the vector
            indices.emplace_back(uniqueVertices[vertex]);
        }
    }
}

void Application::generateMipmaps(
    vk::raii::Image & image,
    vk::Format imageFormat,
    int32_t textureWidth,
    int32_t textureHeight,
    uint32_t mipLevels
) const {
    // Check if linear blitting is supported
    vk::FormatProperties const formatProperties {physicalDevice.getFormatProperties(imageFormat)};
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Texture image support does not support linear blitting!");
    }

    // Proceed with blitting.
    // It works as follows.
    // All mip levels start in layout eTransferDstOptimal, as of entering this function.
    // To blit, we need the level i - 1 to be in the layout eTransferSrcOptimal.
    // A barrier is used to make this transition before blitting.
    // After blitting, the level i - 1 is transitioned to the layout eShaderReadOnlyOptimal,
    // since the texture will be read by the shader in the fragment stage.
    // The last level, which is mipLevels - 1, isn't transitioned to eTransferSrcOptimal, so
    // it is transitioned directly to the layout eShaderReadOnlyOptimal in the end.
    
    vk::raii::CommandBuffer commandBuffer {nullptr};
    beginSingleTimeCommands(commandBuffer);

    vk::ImageMemoryBarrier barrier {
        // Redundant, same values will be set in for loop.
        //.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        //.dstAccessMask = vk::AccessFlagBits::eTransferRead,
        //.oldLayout = vk::ImageLayout::eTransferDstOptimal,
        //.newLayout = vk::ImageLayout::eTransferSrcOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = *image,
        .subresourceRange = vk::ImageSubresourceRange {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    int32_t mipWidth {textureWidth};
    int32_t mipHeight {textureHeight};

    for (uint32_t i {1}; i < mipLevels; ++i) {
        // Transition previous mip level to transfer read
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            {},
            {},
            barrier
        );

        // Make mip map
        vk::ArrayWrapper1D<vk::Offset3D, 2> srcOffsets;
        vk::ArrayWrapper1D<vk::Offset3D, 2> dstOffsets;
        srcOffsets[0] = vk::Offset3D(0, 0, 0);
        srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
        dstOffsets[0] = vk::Offset3D(0, 0, 0);
        dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);

        vk::ImageBlit const blit {
            .srcSubresource = vk::ImageSubresourceLayers {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets = srcOffsets,
            .dstSubresource = vk::ImageSubresourceLayers {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets = dstOffsets
        };

        commandBuffer.blitImage(
            *image,
            vk::ImageLayout::eTransferSrcOptimal,
            *image,
            vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear
        );

        // Transition previous mip level to shader read optimal
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            {},
            {},
            {},
            barrier
        );

        if (mipWidth > 1) {
            mipWidth /= 2;
        }
        if (mipHeight > 1) {
            mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = i;
    }

    // Transition last mip level to shader read optimal.
    // The last mip level isn't used for blitting, so it doesn't transition to eTransferSrcOptimal,
    // it goes straight from eTransferDstOptimal to eShaderReadOnlyOptimal.
    // barrier.subresourceRange.baseMipLevel = mipLevels-1; // assigned in for loop
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {},
        {},
        {},
        barrier
    );

    endSingleTimeCommands(commandBuffer);
}

vk::SampleCountFlagBits Application::getMaxUsableSampleCount() const {
    vk::PhysicalDeviceProperties const physicalDeviceProperties {physicalDevice.getProperties()};

    vk::SampleCountFlags sampleCounts {
        physicalDeviceProperties.limits.framebufferColorSampleCounts &
        physicalDeviceProperties.limits.framebufferDepthSampleCounts
    };

    if (sampleCounts & vk::SampleCountFlagBits::e64) {
        return vk::SampleCountFlagBits::e64;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e32) {
        return vk::SampleCountFlagBits::e32;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e16) {
        return vk::SampleCountFlagBits::e16;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e8) {
        return vk::SampleCountFlagBits::e8;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e4) {
        return vk::SampleCountFlagBits::e4;
    }
    if (sampleCounts & vk::SampleCountFlagBits::e2) {
        return vk::SampleCountFlagBits::e2;
    }

    return vk::SampleCountFlagBits::e1;
}

void Application::createColorResources() {
    vk::Format const colorFormat = swapChainSurfaceFormat.format;

    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        1,
        msaaSamples,
        colorFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        colorImage,
        colorImageMemory
    );

    colorImageView = createImageView(colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
}