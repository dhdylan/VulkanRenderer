#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers =
{
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> physicalDeviceExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

/// <summary>
/// Holds details about the swapchain capabilities of a surface
/// </summary>
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

/// <summary>
/// Utility struct to store Queue families that we'll be using.
/// </summary>
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentationFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentationFamily.has_value();
    }
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

/// <summary>
/// Used to call "VkCreateDebugUtilMessengerEXT". Function address needs to be loaded at runtime since it is an extension. This function uses the same arguments as the actual Vulkan function.
/// </summary>
/// <param name="instance"></param>
/// <param name="pCreateInfo"></param>
/// <param name="pAllocator"></param>
/// <param name="pDebugMessenger"></param>
/// <returns>Will return false if function address couldn't be found.</returns>
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
 
/// <summary>
/// Used to call "VkDestroyDebugUtilMessengerEXT". Function address needs to be loaded at runtime since it is an extension. This function uses the same arguments as the actual Vulkan function.
/// </summary>
/// <param name="instance"></param>
/// <param name="debugMessenger"></param>
/// <param name="pAllocator"></param>
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
    else
    {
        throw std::runtime_error("could not find \"vkDestroyDebugUtilsMessengerEXT\" function address.");
    }
}

class HelloTriangleApplication {
public:

    /// <summary>
    /// Entry point of application.
    /// </summary>
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentationQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    /// <summary>
    /// Just a utility function to fill out the descriptor struct.
    /// </summary>
    /// <param name="createInfo"> - Pass by reference</param>
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    /// <summary>
    /// Gets called whenever there is a validation message. Function signature is determined by Vulkan.
    /// </summary>
    /// <param name="messageSeverity"></param>
    /// <param name="messageType"></param>
    /// <param name="pCallbackData"></param>
    /// <param name="pUserData"></param>
    /// <returns></returns>
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    /// <summary>
    /// Gets all of the required extensions for GLFW to operate with Vulkan.
    /// </summary>
    /// <returns> A vector of all of the necessary extensions as char* strings</returns>
    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    /// <summary>
    /// Iterates through the available surface formats and picks one.
    /// </summary>
    /// <param name="availableFormats"></param>
    /// <returns></returns>
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& format : availableFormats)
        {
            if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                return format;
            }
        }

        return availableFormats[0];
    }

    /// <summary>
    /// Iterates through the available present modes and picks one.
    /// </summary>
    /// <param name="availablePresentModes"></param>
    /// <returns></returns>
    VkPresentModeKHR chooseSwapSurfacePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& presentMode : availablePresentModes)
        {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return presentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /// <summary>
    /// Picks an appropriate swapChain extent based on surface capabilities.
    /// </summary>
    /// <param name="capabilities"></param>
    /// <returns></returns>
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        // have to do this odd parenthese syntax to avoid a member/macro conflict inherent in STL:
        // https://stackoverflow.com/questions/1394132/macro-and-member-function-conflict
        if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) 
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
                
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    /// <summary>
    /// Gets all of the available validation layers that are supported and checks to make sure the one(s) we've requested (in global const "validationLayers") are supported.
    /// </summary>
    /// <returns>Will return false if a requested validation layer is not supported.</returns>
    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        /// loop through each validation layer that we have specified that we want to use,
        /// for each of those check if it exists in the available layers
        /// if it doesn't, then stop and return false because we'd be getting an error when we try to use it
        /// otherwise if all goes well, return true
        for (auto& validationLayer : validationLayers)
        {
            bool layerFound = false;
            for (auto& availableLayer : availableLayers)
            {
                if (strcmp(validationLayer, availableLayer.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    /// <summary>
    /// Initializes GLFW and creates a window. Sotres window in private class member "window"
    /// </summary>
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    /// <summary>
    /// Uses all of the "find____" and "choose____" utility functions to create a swapchain with the proper settings
    /// based on our hardware's capabilities.
    /// </summary>
    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapSurfacePresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.maxImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices queueFamilies = findQueueFamilies(physicalDevice);
        uint32_t familyIndices[] = { queueFamilies.graphicsFamily.value(),  queueFamilies.presentationFamily.value() };

        if (queueFamilies.graphicsFamily != queueFamilies.presentationFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = familyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform; //this is specify no transformation
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain))
        {
            throw std::runtime_error("Failed to create swapchain");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());
        for (int i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image view.");
            }
        }                                                           
    }

    /// <summary>
    /// Calls "vkCreateInstance", stores instance in private member "instance". Checks for validation layer support before doing so.
    /// </summary>
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("Validation layer(s) requested, but not available.");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    /// <summary>
    /// Enumerates through all avilable extensions.
    /// </summary>
    void checkVulkanExtensions()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "Available Vulkan extensions:" << std::endl;

        for (const auto& extension : extensions)
        {
            std::cout << "\t" << extension.extensionName << " | " << extension.specVersion << std::endl;
        }
    }

    /// <summary>
    /// Will attempt to find the index of each of the queue families that are present as members in "QueueFamilyIndices".
    /// </summary>
    /// <param name="device"></param>
    /// <returns></returns>
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.presentationFamily = i;
            }

            if (indices.isComplete())
                break;

            i++;
        }

        return indices;
    }

    /// <summary>
    /// Fills out a "SwapChainSupportDetails" struct
    /// </summary>
    /// <param name="device"></param>
    /// <returns></returns>
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }
        
        return details;
    }

    /// <summary>
    /// Enumerates through all available extension on given physical device, and checks to see if all of the requested extensions (in the const global vector) are present.
    /// </summary>
    /// <param name="physicalDevice"></param>
    /// <returns></returns>
    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionsCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());

        std::set<std::string>requiredExtensions(physicalDeviceExtensions.begin(), physicalDeviceExtensions.end());

        // We have all the available extensions, and we have the required ones, so if we loop through all of the available ones and erase them from the vector of requestedExtensions
        // then we can just check to see if the vector has anything left. If it's empty, that means we're good to go.
        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty(); // If list is empty, they were all checked off.
    }

    /// <summary>
    /// Verifies a device is suitable for our program.
    /// </summary>
    /// <param name="device"></param>
    /// <returns></returns>
    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAcceptable = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails details = querySwapChainSupport(device);
            swapChainAcceptable = !details.formats.empty() && !details.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAcceptable;
    }

    /// <summary>
    /// enumerates through physical devices available and picks one to use as the physical device.
    /// </summary>
    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            throw std::runtime_error("No Vulkan Physical Devices found.");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (auto& device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("No suitable physical devices found.");
        }
    }

    /// <summary>
    /// Creates a logical device with queue families created based off of the "QueueFamilyIndices" struct. Hooks up validation laers in create info struct here as well.
    /// </summary>
    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(physicalDeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = physicalDeviceExtensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("Logical device creation failed.");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentationFamily.value(), 0, &presentationQueue);
    }

    /// <summary>
    /// Uses "glfwCreateSurface" to create a VkSurfaceKHR. Populates private member "surface".
    /// </summary>
    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface))
        {
            throw std::runtime_error("Failed to create window surface.");
        }
    }

    void createGraphicsPipeline()
    {

    }

    /// <summary>
    /// Initializes all of the vulkan resources required to work with the API.
    /// </summary>
    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
    }

    /// <summary>
    /// If necessary, sets up debug messenger so you can see validation messages.
    /// </summary>
    void setupDebugMessenger()
    {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger.");
        }
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup() {
        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();    
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}