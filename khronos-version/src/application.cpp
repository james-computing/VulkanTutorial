#include "../include/application.hpp"

void Application::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Application::initVulkan() {
    createInstance();
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

void Application::getRequiredGLFWExtensions(uint32_t & glfwExtensionCount, char const ** & glfwExtensions) {
    // Get the required instance extensions from GLFW
    char const ** glfwExtensions {glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};

    // Check if the required GLFW extensions are supported by the Vulkan implementation
    vk::ResultValue<std::vector<vk::ExtensionProperties>> resultValueExtensionProperties {context.enumerateInstanceExtensionProperties()};
    if (!resultValueExtensionProperties.has_value()) {
        std::cerr << "Failed to get extension properties in createInstance.";
    }

    std::vector<vk::ExtensionProperties> extensionProperties {resultValueExtensionProperties.value};

    // Print available extensions
    std::cout << "available extensions:\n";
    for (const auto & extensionProperty : extensionProperties) {
        std::cout << '\t' << extensionProperty.extensionName << '\n';
    }

    for (uint32_t i {0}; i < glfwExtensionCount; ++i) {
        // Check if i-th required extension is supported
        bool notSupported {
            std::ranges::none_of(
            extensionProperties,
            [glfwExtension = glfwExtensions[i]](vk::ExtensionProperties const & extensionProperty) -> bool {
                return strcmp(extensionProperty.extensionName, glfwExtension) == 0;
            })
        };

        if (notSupported)
        {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
        }
    }
}

void Application::createInstance() {
    uint32_t glfwExtensionCount;
    char const ** glfwExtensions;
    getRequiredGLFWExtensions(glfwExtensionCount, glfwExtensions);

    constexpr vk::ApplicationInfo appInfo {
        .pApplicationName = "Application",
        .applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0), // VK_MAKE_VERSION is deprecated
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0), // VK_MAKE_VERSION is deprecated
        .apiVersion = vk::ApiVersion14
    };

    vk::InstanceCreateInfo createInfo {
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = glfwExtensionCount,
        .ppEnabledExtensionNames = glfwExtensions
    };

    vk::ResultValue<vk::raii::Instance> resultValueInstance {context.createInstance(createInfo, nullptr)};
    if (!resultValueInstance.has_value()) {
        std::cerr << "Failed to create Vulkan instance in Application::createInstance";
    }

    instance = std::move(resultValueInstance.value);
}