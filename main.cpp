/****************************************************************************
 *                                                                          *
 *  Example of a simple 3D application using Vulkan API                     *
 *  Copyright (C) 2020 Artem Hlumov <artyom.altair@gmail.com>               *
 *                                                                          *
 *  This program is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, either version 3 of the License, or       *
 *  (at your option) any later version.                                     *
 *                                                                          *
 *  This program is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.  *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *       The application shows initialization of Vulkan, creation of        *
 *     vertex buffers, shaders and uniform buffers in order to display      *
 *        a simple example of 3D graphics - rotating colored cube.          *
 *                                                                          *
 *           The code is inspirited of the original tutorial:               *
 *                     https://vulkan-tutorial.com                          *
 *                                                                          *
 ****************************************************************************/

// Include GLFW (window SDK).
// Switch on support of Vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Include GLM (linear algebra library).
// Use radians instead of degrees to disable deprecated functions.
#define GLM_FORCE_RADIANS
// GLM has been initially designed for OpenGL, so we have to apply a patch
// to make it work with Vulkan.
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>

#include <set>
#include <array>
#include <chrono>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <algorithm>

/**
 * Switch on validation levels.
 * Validation levels provided by LunarG display error messages in case of
 * incorrect usage of Vulkan functions.
 * Comment this line to switching off DEBUG_MODE.
 * It makes the application faster but silent.
 */
#define DEBUG_MODE

/**
 * Window width.
 */
constexpr int WINDOW_WIDTH = 800;
/**
 * Window height.
 */
constexpr int WINDOW_HEIGHT = 800;
/**
 * Window title.
 */
constexpr const char* WINDOW_TITLE = "VKExample";
/**
 * Maximal amount of frames processed at the same time.
 */
const int MAX_FRAMES_IN_FLIGHT = 5;

/**
 * Callback function that will be called each time a validation level produces a message.
 * @param messageSeverity Bitmask specifying which severities of events cause a debug messenger callback.
 * @param messageType Bitmask specifying which types of events cause a debug messenger callback.
 * @param pCallbackData Structure specifying parameters returned to the callback.
 * @param pUserData Arbitrary user data provided when the callback is created.
 * @return True in case the Vulkan call should be aborted, false - otherwise.
 */
VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback
    (
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    )
{
    // Mark variables as not used to suppress warnings.
    (void) messageSeverity;
    (void) messageType;
    (void) pUserData;
    // Print the message.
    std::cerr << "[MSG]:" << pCallbackData->pMessage << std::endl;
    // Do only logging, do not abort the call.
    return VK_FALSE;
}

/**
 * Main function.
 * @return Return code of the application.
 */
int main()
{
    // ==========================================================================
    //                 STEP 1: Create a Window using GLFW
    // ==========================================================================
    // GLFW abstracts native calls to create the window and allows us to write
    // a cross-platform application.
    // ==========================================================================

    // Initialize GLFW context.
    glfwInit();
    // Do not create an OpenGL context - we use Vulkan.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Make the window not resizable.
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // Create a window instance.
    GLFWwindow* glfwWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);

    // ==========================================================================
    //                   STEP 2: Select Vulkan extensions
    // ==========================================================================
    // Vulkan has a list of extensions providing some functionality.
    // We should explicitly select extensions we need.
    // At least GLFW requires some graphical capabilities
    // in order to draw an image.
    // ==========================================================================

    // Take a minimal set of Vulkan extensions required by GLWF.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Fetch list of available Vulkan extensions.
    uint32_t vkNumAvailableExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &vkNumAvailableExtensions, nullptr);
    std::vector< VkExtensionProperties > vkAvailableExtensions(vkNumAvailableExtensions);
    vkEnumerateInstanceExtensionProperties(nullptr, &vkNumAvailableExtensions, vkAvailableExtensions.data());

    // Construct a list of extensions we should request.
    std::vector< const char* > extensionsList(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef DEBUG_MODE

    // Add an extension to print debug messages.
    extensionsList.push_back("VK_EXT_debug_utils");

    // ==========================================================================
    //              STEP 2.1: Select validation layers for debug
    // ==========================================================================
    // Validation layers is a mechanism to hook Vulkan API calls, validate
    // them and notify the user if something goes wrong.
    // Here we need to make sure that requested validation layers are available.
    // ==========================================================================

    // Specify desired validation layers.
    const std::vector<const char*> desiredValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // Fetch available validation layers.
    uint32_t vkLayerCount;
    vkEnumerateInstanceLayerProperties(&vkLayerCount, nullptr);
    std::vector< VkLayerProperties > vkAvailableLayers(vkLayerCount);
    vkEnumerateInstanceLayerProperties(&vkLayerCount, vkAvailableLayers.data());

    // Check if desired validation layers are in the list.
    bool validationLayersAvailable = true;
    for (auto requestedLayer : desiredValidationLayers) {
        bool isLayerAvailable = false;
        for (auto availableLayer : vkAvailableLayers) {
            if (std::strcmp(requestedLayer, availableLayer.layerName)) {
                isLayerAvailable = true;
                break;
            }
        }
        if (!isLayerAvailable) {
            validationLayersAvailable = false;
            break;
        }
    }
    if (!validationLayersAvailable) {
        std::cerr << "Desired validation levels are not available!";
        abort();
    }

    // ==========================================================================
    //                STEP 2.2: Create a debug message callback
    // ==========================================================================
    // Validation layers is a mechanism to hook Vulkan API calls, validate
    // them and notify the user if something goes wrong.
    // ==========================================================================

    // Set up message logging.
    // See %VK_SDK_PATH%/Config/vk_layer_settings.txt for detailed information.
    VkDebugUtilsMessengerCreateInfoEXT vkMessangerCreateInfo {};
    vkMessangerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // Which severities of events cause a debug messenger callback.
    vkMessangerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // Which types of events cause a debug messenger callback.
    vkMessangerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    // Pointer to a callback function.
    vkMessangerCreateInfo.pfnUserCallback = messageCallback;
    // Here we can pass some arbitrary date to the callback function.
    vkMessangerCreateInfo.pUserData = nullptr;

#endif

    // ==========================================================================
    // STEP 3: Create a Vulkan instance
    // ==========================================================================
    // The Vulkan instance is a starting point of using Vulkan API.
    // Here we specify API version and which extensions to use.
    // ==========================================================================

    // Specify application info and required Vulkan version.
    VkApplicationInfo vkAppInfo {};
    // Information about your application.
    vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkAppInfo.pApplicationName = WINDOW_TITLE;
    vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    // Information about your 3D engine (if applicable).
    vkAppInfo.pEngineName = "No Engine";
    vkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // Use v1.0 that is likely supported by the most of drivers.
    vkAppInfo.apiVersion = VK_API_VERSION_1_0;

    // Fill in an instance create structure.
    VkInstanceCreateInfo vkCreateInfo {};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkCreateInfo.pApplicationInfo = &vkAppInfo;
    // Specify which extensions we need.
    vkCreateInfo.enabledExtensionCount = extensionsList.size();
    vkCreateInfo.ppEnabledExtensionNames = extensionsList.data();
#ifdef DEBUG_MODE
    // Switch on all requested layers for debug mode.
    vkCreateInfo.enabledLayerCount = static_cast< uint32_t >(desiredValidationLayers.size());
    vkCreateInfo.ppEnabledLayerNames = desiredValidationLayers.data();
    // Debug message callbacks are attached after instance creation and should be destroyed
    // before instance destruction. Therefore they do not catch errors in these two calls.
    // To avoid this we can pass our debug messager duing instance creation.
    // This will apply it for vkCreateInstance() and vkDestroyInstance() calls only,
    // so in any case debug messages should be switched on after the instance is created.
    vkCreateInfo.pNext = reinterpret_cast< VkDebugUtilsMessengerCreateInfoEXT* >(&vkMessangerCreateInfo);
#else
    // Do not use layers in release mode.
    vkCreateInfo.enabledLayerCount = 0;
#endif

    // Create a Vulkan instance and check its validity.
    VkInstance vkInstance;
    if (vkCreateInstance(&vkCreateInfo, nullptr, &vkInstance) != VK_SUCCESS) {
        std::cerr << "Cannot creae a Vulkan instance!";
        abort();
    }

    // ==========================================================================
    // STEP 4: Create a window surface
    // ==========================================================================
    // Surface is an abstraction that works with the window system of your OS.
    // Athough it is possible to use platform-dependent calls to create
    // a surface, GLFW provides us a way to do this platform-agnostic.
    // ==========================================================================

    VkSurfaceKHR vkSurface;
    if (glfwCreateWindowSurface(vkInstance, glfwWindow, nullptr, &vkSurface) != VK_SUCCESS) {
        std::cerr << "Failed to create a surface!" << std::endl;
        abort();
    }

#ifdef DEBUG_MODE

    // ==========================================================================
    // STEP 5: Attach message handler
    // ==========================================================================
    // Attach a message handler to the Vulkan context in order to see
    // debug messages and warnings.
    // ==========================================================================

    // Since vkCreateDebugUtilsMessengerEXT is a function of an extension,
    // it is not explicitly declared in Vulkan header, because if we do not load
    // validation layers, it does not exist.
    // First of all we have to obtain a pointer to the function.
    auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast< PFN_vkCreateDebugUtilsMessengerEXT >(vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT"));
    if (vkCreateDebugUtilsMessengerEXT == nullptr) {
        std::cerr << "Function vkCreateDebugUtilsMessengerEXT not found!";
        abort();
    }

    // Create a messenger.
    VkDebugUtilsMessengerEXT vkDebugMessenger;
    vkCreateDebugUtilsMessengerEXT(vkInstance, &vkMessangerCreateInfo, nullptr, &vkDebugMessenger);

#endif

    // ==========================================================================
    // STEP 6: Pick a physical device
    // ==========================================================================
    // Physical devices correspond to graphical cards available in the system.
    // Before we continue, we should make sure the graphical card is suitable
    // for our needs and in case there is more than one card
    // in the system - select one of them.
    // We are going to perform some tests and fetch device information that
    // we will need after. So, to not request this information twice,
    // we will fill queueFamilyIndices and swapChainSupportDetails
    // for the selected physical device and use them in further calls.
    // ==========================================================================

    // Here we will store a selected physical device.
    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    // Here we store information about queues supported by the selected physical device.
    // For our simple application device should support two queues:
    struct QueueFamilyIndices
    {
        // Graphics queue processes rendering requests and stores result into Vulkan images.
        std::optional< uint32_t > graphicsFamily;
        // Present queue tranfers images to the surface.
        std::optional< uint32_t > presentFamily;
    };
    QueueFamilyIndices queueFamilyIndices;
    // Here we take information about a swap chain.
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector< VkSurfaceFormatKHR > formats;
        std::vector< VkPresentModeKHR > presentModes;
    };
    SwapChainSupportDetails swapChainSupportDetails;
    // Here we keep a selected format for z-buffer.
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    // Desired extensions that should be supported by the graphical card.
    const std::vector< const char* > desiredDeviceExtensions = {
        // Swap chain extension is needed for drawing.
        // Any graphical card that aims to draw into a framebuffer
        // should support this extension.
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // Get a list of available physical devices.
    uint32_t vkDeviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, nullptr);
    if (vkDeviceCount == 0) {
        std::cerr << "No physical devices!" << std::endl;
        abort();
    }
    std::vector<VkPhysicalDevice> vkDevices(vkDeviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, vkDevices.data());

    // Go through the list of physical device and select the first suitable one.
    // In advanced applications you may introduce a rating to choose
    // the best video card or let the user select one manually.
    for (auto device : vkDevices) {
        // Get properties of the physical device.
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Get features of the physical device.
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // Get extensions available for the physical device.
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector< VkExtensionProperties > availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // Check if all desired extensions are present.
        std::set< std::string > requiredExtensions(desiredDeviceExtensions.begin(), desiredDeviceExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        bool allExtensionsAvailable = requiredExtensions.empty();

        // Get list of available queue families.
        uint32_t vkQueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &vkQueueFamilyCount, nullptr);
        std::vector< VkQueueFamilyProperties > vkQueueFamilies(vkQueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &vkQueueFamilyCount, vkQueueFamilies.data());

        // Fill in QueueFamilyIndices structure to check that all required queue families are present.
        QueueFamilyIndices currentDeviceQueueFamilyIndices;
        for (uint32_t i = 0; i < vkQueueFamilyCount; i++) {
            // Take a queue family.
            const auto& queueFamily = vkQueueFamilies[i];

            // Check if this is a graphics family.
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                currentDeviceQueueFamilyIndices.graphicsFamily = i;
            }

            // Check if it supports presentation.
            // Note that graphicsFamily and presentFamily may refer to the same queue family
            // for some video cards and we should be ready to this.
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkSurface, &presentSupport);
            if (presentSupport) {
                currentDeviceQueueFamilyIndices.presentFamily = i;
            }
        }
        bool queuesOk = currentDeviceQueueFamilyIndices.graphicsFamily.has_value() &&
                        currentDeviceQueueFamilyIndices.presentFamily.has_value();

        // Fill swap chain information into a SwapChainSupportDetails structure.
        SwapChainSupportDetails currenDeviceSwapChainDetails;
        // We should do this only in case the device supports swap buffer.
        // To avoid extra flags and more complex logic, just make sure that all
        // desired extensions have been found.
        if (allExtensionsAvailable) {
            // Get surface capabilities.
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkSurface, &currenDeviceSwapChainDetails.capabilities);
            // Get supported formats.
            uint32_t physicalDeviceFormatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &physicalDeviceFormatCount, nullptr);
            if (physicalDeviceFormatCount != 0) {
                currenDeviceSwapChainDetails.formats.resize(physicalDeviceFormatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &physicalDeviceFormatCount, currenDeviceSwapChainDetails.formats.data());
            }
            // Get supported present modes.
            uint32_t physicalDevicePresentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &physicalDevicePresentModeCount, nullptr);
            if (physicalDevicePresentModeCount != 0) {
                currenDeviceSwapChainDetails.presentModes.resize(physicalDevicePresentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &physicalDevicePresentModeCount, currenDeviceSwapChainDetails.presentModes.data());
            }
        }
        bool swapChainOk = !currenDeviceSwapChainDetails.formats.empty() &&
                           !currenDeviceSwapChainDetails.presentModes.empty();

        // Select a format of z-buffer.
        // We have a list of formats we need to test and pick one.
        std::vector< VkFormat > depthFormatCandidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        VkFormat currentDepthFormat = VK_FORMAT_UNDEFINED;
        for (VkFormat format : depthFormatCandidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(device, format, &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                currentDepthFormat = format;
                break;
            }
        }
        bool depthFormatOk = (currentDepthFormat != VK_FORMAT_UNDEFINED);

        // Select the first suitable device.
        if (vkPhysicalDevice == VK_NULL_HANDLE && allExtensionsAvailable && queuesOk && swapChainOk && depthFormatOk) {
            vkPhysicalDevice = device;
            queueFamilyIndices = currentDeviceQueueFamilyIndices;
            swapChainSupportDetails = currenDeviceSwapChainDetails;
            depthFormat = currentDepthFormat;
        }
    }

    // Check if we have found any suitable device.
    if (vkPhysicalDevice == VK_NULL_HANDLE) {
        std::cerr << "No suitable physical devices available!" << std::endl;
        abort();
    }

    // ==========================================================================
    // STEP 5: Create a logical device
    // ==========================================================================
    // Logical devices are instances of the physical device created for
    // the particular application. We should create one in order to use it.
    // ==========================================================================

    // Logical device of a video card.
    VkDevice vkDevice;

    // As it was mentioned above, we might have two queue families referring
    // to the same index which means there is only one family that is suitable
    // for both needs.
    // Use std::set to filter out duplicates as we should mention each queue
    // only once during logical device creation.
    std::set< uint32_t > uniqueQueueFamilies = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()
    };

    // Go through all remaining queues and make a creation info structure.
    std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Select physical device features we want to use.
    // As this is a quite simple application we need nothing special.
    // However, for more compex applications you might need to first
    // check if device supports features you need via
    // vkGetPhysicalDeviceFeatures() call in physical device suitability check.
    // If you specify something that is not supported - device
    // creation will fail, so you should check beforehand.
    VkPhysicalDeviceFeatures vkDeviceFeatures {};

    // Logical device creation info.
    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &vkDeviceFeatures;
    // Specify which extensions we want to enable.
    createInfo.enabledExtensionCount = static_cast< uint32_t >(desiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = desiredDeviceExtensions.data();

#ifdef DEBUG_MODE

    // Switch on all requested layers for debug mode.
    createInfo.enabledLayerCount = static_cast< uint32_t >(desiredValidationLayers.size());
    createInfo.ppEnabledLayerNames = desiredValidationLayers.data();
#else

    createInfo.enabledLayerCount = 0;

#endif

    // Create a logical device.
    if (vkCreateDevice(vkPhysicalDevice, &createInfo, nullptr, &vkDevice) != VK_SUCCESS) {
        std::cerr << "Failed to create a logical device!" << std::endl;
        abort();
    }

    // ==========================================================================
    // STEP 5: Pick a graphics queue
    // ==========================================================================
    // We have checked that the device supports all required queues, but now
    // we need to pick their handles explicitly.
    // ==========================================================================

    // Pick a graphics queue.
    VkQueue vkGraphicsQueue;
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.graphicsFamily.value(), 0, &vkGraphicsQueue);

    // Pick a present queue.
    // It might happen that both handles refer to the same queue.
    VkQueue vkPresentQueue;
    vkGetDeviceQueue(vkDevice, queueFamilyIndices.presentFamily.value(), 0, &vkPresentQueue);

    // ==========================================================================
    // STEP 5: Select surface configuration
    // ==========================================================================
    // We should select surface format, present mode and extent (size) from
    // the proposed values. They will be used in furhter calls.
    // ==========================================================================

    // Select a color format.
    VkSurfaceFormatKHR vkSelectedFormat = swapChainSupportDetails.formats[0];
    for (const auto& availableFormat : swapChainSupportDetails.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            vkSelectedFormat = availableFormat;
            break;
        }
    }

    // Select a present mode.
    VkPresentModeKHR vkSelectedPresendMode = swapChainSupportDetails.presentModes[0];
    for (const auto& availablePresentMode : swapChainSupportDetails.presentModes) {
        // Preferrable mode. If we find it, break the cycle immediately.
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            vkSelectedPresendMode = availablePresentMode;
            break;
        }
        // Fallback mode. If we find it, store to vkSelectedPresendMode,
        // but try to find something better.
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
            vkSelectedPresendMode = availablePresentMode;
        }
    }

    // Select a swap chain images resolution.
    VkExtent2D vkSelectedExtent;
    if (swapChainSupportDetails.capabilities.currentExtent.width != UINT32_MAX) {
        vkSelectedExtent = swapChainSupportDetails.capabilities.currentExtent;
    } else {
        // Some window managers do not allow to use resolution different from
        // the resolution of the window. In such cases Vulkan will report
        // UINT32_MAX as currentExtent.width and currentExtent.height.
        vkSelectedExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };
        // Make sure the value is between minImageExtent.width and maxImageExtent.width.
        vkSelectedExtent.width = std::max(
                    swapChainSupportDetails.capabilities.minImageExtent.width,
                    std::min(
                        swapChainSupportDetails.capabilities.maxImageExtent.width,
                        vkSelectedExtent.width
                        )
                    );
        // Make sure the value is between minImageExtent.height and maxImageExtent.height.
        vkSelectedExtent.height = std::max(
                    swapChainSupportDetails.capabilities.minImageExtent.height,
                    std::min(
                        swapChainSupportDetails.capabilities.maxImageExtent.height,
                        vkSelectedExtent.height
                        )
                    );
    }

    // ==========================================================================
    // STEP 5: Create a swap chain
    // ==========================================================================
    // Swap chain is a chain of rendered images that are going to be displayed
    // on the screen. It is used to synchronize images rendering with refresh
    // rate of the screen (VSync). If the application generates frames faster
    // than they are displayed, it should wait.
    // ==========================================================================

    // First of all we should select a size of the swap chain.
    // It is recommended to use minValue + 1 but we also have to make sure
    // it does not exceed maxValue.
    // If maxValue is zero, it means there is no upper bound.
    uint32_t imageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
    if (swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapChainSupportDetails.capabilities.maxImageCount) {
        imageCount = swapChainSupportDetails.capabilities.maxImageCount;
    }

    // Fill in swap chain create info using selected surface configuration.
    VkSwapchainCreateInfoKHR vkSwapChainCreateInfo{};
    vkSwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vkSwapChainCreateInfo.surface = vkSurface;
    vkSwapChainCreateInfo.minImageCount = imageCount;
    vkSwapChainCreateInfo.imageFormat = vkSelectedFormat.format;
    vkSwapChainCreateInfo.imageColorSpace = vkSelectedFormat.colorSpace;
    vkSwapChainCreateInfo.imageExtent = vkSelectedExtent;
    vkSwapChainCreateInfo.imageArrayLayers = 1;
    vkSwapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // We have two options for queue synchronization:
    // - VK_SHARING_MODE_EXCLUSIVE - An image ownership should be explicitly transferred
    //                               before using it in a differen queue. Best performance option.
    // - VK_SHARING_MODE_CONCURRENT - Images can be used in different queues without
    //                                explicit ownership transfer. Less performant, but simpler in implementation.
    // If we have only one queue family - we should use VK_SHARING_MODE_EXCLUSIVE as we do not need
    // to do any synchrnoization and can use the faster option for free.
    // If we have two queue families - we will use VK_SHARING_MODE_CONCURRENT mode to avoid
    // additional complexity of ownership transferring.
    uint32_t familyIndices[] = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()
    };
    if (familyIndices[0] != familyIndices[1]) {
        vkSwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        vkSwapChainCreateInfo.queueFamilyIndexCount = 2;
        vkSwapChainCreateInfo.pQueueFamilyIndices = familyIndices;
    } else {
        vkSwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkSwapChainCreateInfo.queueFamilyIndexCount = 0;
        vkSwapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    vkSwapChainCreateInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
    vkSwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vkSwapChainCreateInfo.presentMode = vkSelectedPresendMode;
    vkSwapChainCreateInfo.clipped = VK_TRUE;
    // This option is only required if we recreate a swap chain.
    vkSwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create a swap chain.
    VkSwapchainKHR vkSwapChain;
    if (vkCreateSwapchainKHR(vkDevice, &vkSwapChainCreateInfo, nullptr, &vkSwapChain) != VK_SUCCESS) {
        std::cerr << "Failed to create a swap chain!" << std::endl;
        abort();
    }

    // ==========================================================================
    // STEP 5: Load shaders
    // ==========================================================================
    // Shaders are special code that is executed directly on GPU.
    // For our simple example we use vertex and fragment shaders.
    // Unlike OpenGL, Vulcan uses a binary format which is called SPIR-V and
    // each shader should be compiled to this format using glslc compiler that
    // could be found in Vulkan SDK.
    // ==========================================================================

    // Create a helper function that loads a shader since we need two of them.
    auto createShaderModule = [=](const char* fileName) {
        // Open file.
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Shader file " << fileName << " not found!" << std::endl;
            abort();
        }
        // Calculate file size.
        size_t fileSize = static_cast< size_t >(file.tellg());
        // Jump to the beginning of the file.
        file.seekg(0);
        // Read shader code.
        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);
        // Close the file.
        file.close();
        // Shader module creation info.
        VkShaderModuleCreateInfo vkShaderCreateInfo{};
        vkShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkShaderCreateInfo.codeSize = buffer.size();
        vkShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(buffer.data());
        // Create a shader module.
        VkShaderModule vkShaderModule;
        if (vkCreateShaderModule(vkDevice, &vkShaderCreateInfo, nullptr, &vkShaderModule) != VK_SUCCESS) {
            std::cerr << "Cannot create a shader!" << std::endl;
            abort();
        }
        // Return a shader module handle.
        return vkShaderModule;
    };

    // Load two shaders for our simple example.
    VkShaderModule vkVertexShaderModule = createShaderModule("main.vert.spv");
    VkShaderModule vkFragmentShaderModule = createShaderModule("main.frag.spv");

    // Create a pipeline stage for a vertex shader.
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vkVertexShaderModule;
    // main function name
    vertShaderStageInfo.pName = "main";

    // Create a pipeline stage for a fragment shader.
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = vkFragmentShaderModule;
    // main function name
    fragShaderStageInfo.pName = "main";

    // Put both shader modules into an array.
    std::array< VkPipelineShaderStageCreateInfo, 2 > shaderStages {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    // ==========================================================================
    // STEP 5: Create a uniform buffer
    // ==========================================================================
    // Uniform buffers are used to provide variables to shaders.
    // For example, in our case such variables are transformation matrices.
    // ==========================================================================

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create a descriptor set layout" << std::endl;
        abort();
    }

    // ==========================================================================
    // STEP 5: Create a vertex buffer
    // ==========================================================================
    // Vertex buffers provide vertices to shaders.
    // In our case this is a geometry of a cube.
    // ==========================================================================

    // Structure that represents a vertex of a cube.
    struct Vertex
    {
        // 3D coordinates of the vertex.
        glm::vec3 pos;
        // Color of the vertex.
        glm::vec3 color;
    };

    // Create a cube specifying its vertices.
    // Each triplet of vertices represent one triangle.
    // We do not use index buffer, so some vertices are duplicated.
    // Each plane has its own color.
    std::vector< Vertex > vertices
    {
        { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },

        { { -0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 0.0f } },

        { {  0.5f, -0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f } },
        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f } },

        { { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f } },

        { {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f } },
        { { -0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f } },
        { {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 1.0f } },
        { { -0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } },

        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f } },
        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
        { { -0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } },
        { {  0.5f, -0.5f,  0.5f }, { 0.0f, 1.0f, 1.0f } },
    };

    // Binding descriptor specifies how our array is splited into vertices.
    // In particular, we say that each sizeof(Vertex) bytes correspond to one vertex.
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Attribute description specifies how one vertext is split into separate variables.
    // In our case a vertext is a composition of two vec3 values: coordinates and color.
    // Property location corresponds to a location value in shader code.
    std::array< VkVertexInputAttributeDescription, 2 > attributeDescriptions{};
    // Description of the first attribute (coordinates).
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    // Description of the second attribute (color).
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    // Create a vertex input state for pipeline creation.
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast< uint32_t >(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // ==========================================================================
    // STEP 5: Create a pipeline assembly state
    // ==========================================================================
    // Pipeline assembly state describes a geometry of the input data.
    // In our case the input is a list of triangles.
    // ==========================================================================

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // ==========================================================================
    // STEP 5: Create a viewport and scissors
    // ==========================================================================
    // Viewport is a region of a framebuffer that will be used for renderring.
    // Scissors define if some part of rendered image should be cut.
    // In our example we define both viewport and scissors equal to
    // the framebuffer size.
    // ==========================================================================

    // Create a viewport.
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast< float >(vkSelectedExtent.width);
    viewport.height = static_cast< float >(vkSelectedExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Create scissors.
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vkSelectedExtent;

    // Make a structure for framebuffer creation.
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // ==========================================================================
    // STEP 5: Create a rasterization stage
    // ==========================================================================
    // Rasterization stage takes primitives and rasterizes them to fragments
    // pased to the fragment shader.
    // ==========================================================================

    // Rasterizer create info
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    // Fill in triangles.
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    // Enable face culling.
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // ==========================================================================
    // STEP 5: Create a MSAA state
    // ==========================================================================
    // MultiSample Anti-Aliasing is used to make edges smoother by rendering
    // them in higher resolution (having more then one fragment per pixel).
    // In our example we do not use this feature.
    // ==========================================================================

    // Create a state for MSAA.
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // ==========================================================================
    // STEP 5: Create a pipeline color blend
    // ==========================================================================
    // Color blend state describes how fragments are applied to the result
    // image. There might be options like mixing, but we switch off blending and
    // simply put a new color instead of existing one.
    // ==========================================================================

    // Configuration per attached framebuffer.
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    // Other fields are optional.
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // Global color blending settings.
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.logicOpEnable = VK_FALSE;
    // Other fields are optional.
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // ==========================================================================
    // STEP 5: Create a color attachment
    // ==========================================================================
    // Color attachment contains bytes of rendered image, so we should create
    // one in order to display something.
    // ==========================================================================

    // Descriptor of a color attachment.
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vkSelectedFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Color attachment reference.
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // ==========================================================================
    // STEP 5: Create a depth and stensil attachment
    // ==========================================================================
    // Depth and stensil attachment is used to support two tests:
    // - depth test - Makes sure that only the nearest fragment is displayed.
    // - stensil test - Allows to cut some fragments depending on values of
    //                  the stensil buffer.
    // Although in this example we only need a depth test, both of them use
    // the same attachment.
    // ==========================================================================

    // Descriptor of a depth attachment.
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Depth attachment reference.
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // ==========================================================================
    // STEP 5: Configure depth and stensil tests
    // ==========================================================================
    // Depth and stensil attachment is created and now we need to configure
    // these tests. In this example we use regular VK_COMPARE_OP_LESS depth
    // operation and disable stensil test.
    // ==========================================================================

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = VkStencilOpState{};
    depthStencil.back = VkStencilOpState{};

    // ==========================================================================
    // STEP 5: Create a render pass
    // ==========================================================================
    // Render pass represents a collection of attachments, subpasses
    // and dependencies between the subpasses.
    // Subpasses allow to organize rendering process as a chain of operations.
    // Each operation is applied to the result of the previous one.
    // In the example we only need a single subpass.
    // ==========================================================================

    // Define a subpass and include both attachments (color and depth-stensil).
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Define a subpass dependency.
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Define a render pass and attach the subpass.
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    std::array< VkAttachmentDescription, 2 > attachments = { colorAttachment, depthAttachment };
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    // Create a render pass.
    VkRenderPass renderPass;
    if (vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::cerr << "Failed to create a render pass!" << std::endl;
        abort();
    }

    // ==========================================================================
    // STEP 5: Create a graphics pipeline
    // ==========================================================================
    // All stages prepared above should be combined into a graphics pipeline.
    // ==========================================================================

    // Define a pipeline layout.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    // Create a pipeline layout.
    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to creare a pipeline layout!" << std::endl;
        abort();
    }

    // Define a pipeline and provide all stages created above.
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    // Create a pipeline.
    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::cerr << "Cannot create a graphics pipeline!" << std::endl;
        abort();
    }











    auto findMemoryType = [=](uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        std::cerr << "No suitable memory type!" << std::endl;
        abort();
    };





    // ==========================================================================
    // STEP 5: Create swap chain image views
    // ==========================================================================
    // After the swap chain is created, it contains Vulkan images that are
    // used to transfer rendered picture. In order to work with images
    // we should create image views.
    // ==========================================================================

    // Fetch Vulkan images associated to the swap chain.
    std::vector< VkImage > vkSwapChainImages;
    uint32_t vkSwapChainImageCount;
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &vkSwapChainImageCount, nullptr);
    vkSwapChainImages.resize(vkSwapChainImageCount);
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &vkSwapChainImageCount, vkSwapChainImages.data());

    // Create image views for each image.
    std::vector< VkImageView > swapChainImageViews;
    swapChainImageViews.resize(vkSwapChainImageCount);
    for (size_t i = 0; i < vkSwapChainImageCount; i++) {
        // Image view create info.
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vkSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vkSelectedFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        // Create an image view.
        if (vkCreateImageView(vkDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create an image view #" << i << "!" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    // STEP 5: Create a depth buffer image
    // ==========================================================================
    // In order to use a depth buffer, we should create an image.
    // Unlike swap buffer images, we need only one depth image and it should
    // be created explicitly.
    // ==========================================================================

    // Describe a depth image.
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = vkSelectedExtent.width;
    imageInfo.extent.height = vkSelectedExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a depth image.
    VkImage depthImage;
    if (vkCreateImage(vkDevice, &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
        std::cerr << "Cannot create a depth image!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the depth image.
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vkDevice, depthImage, &memRequirements);

    // Define memory allocate info.
    VkMemoryAllocateInfo memoryAllocInfo{};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memRequirements.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Allocate memory for the depth image.
    VkDeviceMemory depthImageMemory;
    if (vkAllocateMemory(vkDevice, &memoryAllocInfo, nullptr, &depthImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    // Bind the image to the allocated memory.
    vkBindImageMemory(vkDevice, depthImage, depthImageMemory, 0);

    // ==========================================================================
    // STEP 5: Create a depth buffer image view
    // ==========================================================================
    // Similarly to swap buffer images, we need an image view to use it.
    // ==========================================================================

    // Destribe an image view.
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    // Create an image view.
    VkImageView depthImageView;
    if (vkCreateImageView(vkDevice, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    // ==========================================================================
    // STEP 5: Create framebuffers
    // ==========================================================================
    // Framebuffer refers to all attachments that are output of the rendering
    // process.
    // ==========================================================================

    // Create framebuffers.
    std::vector< VkFramebuffer > swapChainFramebuffers;
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        // We have only two attachments: color and depth.
        // Depth attachment is shared.
        std::array< VkImageView, 2 > attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        // Describe a framebuffer.
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
        framebufferInfo.pAttachments =  attachments.data();
        framebufferInfo.width = vkSelectedExtent.width;
        framebufferInfo.height = vkSelectedExtent.height;
        framebufferInfo.layers = 1;

        // Create a framebuffer.
        if (vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a framebuffer!" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    // STEP 5: Create a vertex buffer
    // ==========================================================================
    // Vertex buffer contains vertices of our model we want to pass
    // to the vertex shader.
    // ==========================================================================

    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

    // Describe a buffer.
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = vertexBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a buffer.
    VkBuffer vertexBuffer;
    if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to create a vertex buffer!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the vertex buffer.
    VkMemoryRequirements vertexBufferMemRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vertexBuffer, &vertexBufferMemRequirements);

    // Define memory allocate info.
    VkMemoryAllocateInfo vertexBufferAllocInfo{};
    vertexBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertexBufferAllocInfo.allocationSize = vertexBufferMemRequirements.size;
    vertexBufferAllocInfo.memoryTypeIndex = findMemoryType(vertexBufferMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Allocate memory for the vertex buffer.
    VkDeviceMemory vertexBufferMemory;
    if (vkAllocateMemory(vkDevice, &vertexBufferAllocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memroy for the vertex buffer!" << std::endl;
        abort();
    }

    // Bind the buffer to the allocated memory.
    vkBindBufferMemory(vkDevice, vertexBuffer, vertexBufferMemory, 0);

    // Copy our vertices to the allocated memory.
    void* vertexBufferMemoryData;
    vkMapMemory(vkDevice, vertexBufferMemory, 0, vertexBufferSize, 0, &vertexBufferMemoryData);
    memcpy(vertexBufferMemoryData, vertices.data(), (size_t) vertexBufferSize);
    vkUnmapMemory(vkDevice, vertexBufferMemory);

    // ==========================================================================
    // STEP 5: Create uniform buffers
    // ==========================================================================
    // Uniform buffer contains structures that are provided to shaders
    // as uniform variable. In our case this is a couple of matrices.
    // As we expect to have more than one frame rendered at the same time
    // and we are going to update this buffer every frame, we should avoid
    // situation when one frame reads the uniform buffer while it is
    // being updated. So we should create one buffer per swap chain image.
    // ==========================================================================

    // Structure that we want to provide to the vertext shader.
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    // Get size of the uniform buffer.
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // Uniform buffers.
    std::vector< VkBuffer > uniformBuffers;
    uniformBuffers.resize(vkSwapChainImages.size());

    // Memory of uniform buffers.
    std::vector< VkDeviceMemory > uniformBuffersMemory;
    uniformBuffersMemory.resize(vkSwapChainImages.size());

    // Create one uniform buffer per swap chain image.
    for (size_t i = 0; i < vkSwapChainImages.size(); i++) {
        // Describe a buffer.
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Create a buffer.
        if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &uniformBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        // Retrieve memory requirements for the vertex buffer.
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vkDevice, uniformBuffers[i], &memRequirements);

        // Define memory allocate info.
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Allocate memory for the vertex buffer.
        if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &uniformBuffersMemory[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        // Bind the buffer to the allocated memory.
        vkBindBufferMemory(vkDevice, uniformBuffers[i], uniformBuffersMemory[i], 0);
    }

    // ==========================================================================
    // STEP 5: Create descriptopr sets
    // ==========================================================================
    // In order to use uniforms, we should create a descriptor set for each
    // uniform buffer. Descriptors are allocated from the descriptor poll,
    // so we should create it first.
    // ==========================================================================

    // Define a descriptor pool size. We need one descriptor set per swap chain image.
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(vkSwapChainImages.size());

    // Define descriptor pool.
    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = &poolSize;
    descriptorPoolInfo.maxSets = static_cast< uint32_t >(vkSwapChainImages.size());

    // Create descriptor pool.
    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(vkDevice, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        std::cerr << "Failed to create a descriptor pool!" << std::endl;
        abort();
    }

    // Take a descriptor set layout created above and use it for all descriptor sets.
    std::vector< VkDescriptorSetLayout > layouts(vkSwapChainImages.size(), descriptorSetLayout);

    // Describe allocate infor for descriptor set.
    VkDescriptorSetAllocateInfo descriptSetAllocInfo{};
    descriptSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptSetAllocInfo.descriptorPool = descriptorPool;
    descriptSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(vkSwapChainImages.size());
    descriptSetAllocInfo.pSetLayouts = layouts.data();

    // Create a descriptor set.
    std::vector< VkDescriptorSet > descriptorSets;
    descriptorSets.resize(vkSwapChainImages.size());
    if (vkAllocateDescriptorSets(vkDevice, &descriptSetAllocInfo, descriptorSets.data()) != VK_SUCCESS) {
        // TODO: replace exceptions
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // Write descriptors for each uniform buffer.
    for (size_t i = 0; i < vkSwapChainImages.size(); i++) {
        // Describe a uniform buffer info.
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // Describe a descriptor set to write.
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        // Write the descriptor set.
        vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
    }

    // ==========================================================================
    // STEP 5: Create command buffers
    // ==========================================================================
    // Command buffers describe a set of rendering commands submitted to Vulkan.
    // We need to have one buffer per each image in the swap chain.
    // Command buffers are taken from the command pool, so we should
    // create one.
    // ==========================================================================

    // Describe a command pool.
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0;

    // Create a command pool.
    VkCommandPool commandPool;
    if (vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        std::cerr << "Failed to create a command pool!" << std::endl;
        abort();
    }

    // Create a vector for all command buffers.
    std::vector< VkCommandBuffer > commandBuffers;
    commandBuffers.resize(swapChainFramebuffers.size());

    // Describe a command buffer allocate info.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast< uint32_t >(commandBuffers.size());

    // Allocate command buffers.
    if (vkAllocateCommandBuffers(vkDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        std::cerr << "Failed to create command buffers" << std::endl;
        abort();
    }

    // Describe a rendering sequence for each command buffer.
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        // Start adding commands into the buffer.
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            std::cerr << "Failed to start command buffer recording" << std::endl;
            abort();
        }

        // Define default values of color and depth buffer attachment elements.
        // In our case this means a black color of the background and a maximal depth of each fragment.
        std::array< VkClearValue, 2 > clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        // Describe a render pass.
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = vkSelectedExtent;
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        // Start render pass.
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        // Bind a pipeline we defined above.
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        // Bind vertices.
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
        // Bind descriptor sets for uniforms.
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
        // Draw command.
        vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
        // Finish render pass.
        vkCmdEndRenderPass(commandBuffers[i]);

        // Fihish adding commands into the buffer.
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to finish command buffer recording" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    // STEP 5: Synchronization primitives
    // ==========================================================================
    // Rendering and presentation are not synchronized. It means that if the
    // application renders frames faster then they are displayed, it will lead
    // to memory overflow. In order to avoid this, we should wait if
    // rendering goes too fast and the chain is overflown.
    // ==========================================================================

    // Describe a semaphore.
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // The first semaphore group signals that an image is aquired and ready for rendering.
    // Create semaphore per each image we expect to render in parallel.
    // These semaphores perform GPU-GPU synchronization.
    std::vector< VkSemaphore > imageAvailableSemaphores;
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Create a semaphore.
        if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a semaphore!" << std::endl;
            abort();
        }
    }

    // The second semaphore group signals that an image is rendered and ready for presentation.
    // Create semaphore per each image we expect to render in parallel.
    // These semaphores perform GPU-GPU synchronization.
    std::vector< VkSemaphore > renderFinishedSemaphores;
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Create a semaphore.
        if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a semaphore!" << std::endl;
            abort();
        }
    }

    // In order to not overflow the swap chain we need to wait on CPU side if there are too many images
    // produced by GPU. This CPU-GPU synchronization is performed by fences.

    // Free fences for images running in parallel.
    std::vector<VkFence> inFlightFences;
    // Buffer of feces, locked by the images running in parallel.
    std::vector<VkFence> imagesInFlight;

    // Describe a fence.
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create fences.
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(vkSwapChainImages.size(), VK_NULL_HANDLE);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(vkDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a fence!" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    // STEP 5: Main loop
    // ==========================================================================
    // Main loop performs event hanlding and executes rendering.
    // ==========================================================================

    // Index of a framce processed in the current loop.
    // We go through MAX_FRAMES_IN_FLIGHT indices.
    size_t currentFrame = 0;

    // Initial value of the system timer we use for rotation animation.
    auto startTime = std::chrono::high_resolution_clock::now();

    // Main loop.
    while(!glfwWindowShouldClose(glfwWindow)) {
        // Poll GLFW events.
        glfwPollEvents();

        // Wait for the current frame.
        vkWaitForFences(vkDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        // Aquire a next image from a swap chain to process.
        uint32_t imageIndex;
        vkAcquireNextImageKHR(vkDevice, vkSwapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // Calculate time difference and rotation angle.
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration< float, std::chrono::seconds::period >(currentTime - startTime).count();
        float angle = time * glm::radians(90.0f);

        // Update uniform buffer object.
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        float aspectRatio = static_cast< float >(vkSelectedExtent.width) / vkSelectedExtent.height;
        ubo.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);

        // Write the uniform buffer object.
        void* data;
        vkMapMemory(vkDevice, uniformBuffersMemory[imageIndex], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(vkDevice, uniformBuffersMemory[imageIndex]);

        // If the image is locked - wait for it.
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(vkDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        // Put a free fence to imagesInFlight array.
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        // Describe a submit to the graphics queue.
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // Specify semaphores the GPU should wait before executing the submit.
        std::array< VkSemaphore, 1 > waitSemaphores{ imageAvailableSemaphores[currentFrame] };
        // Pipeline stages corresponding to each semaphore.
        std::array< VkPipelineStageFlags, 1 > waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        // Specify semaphores the GPU should unlock after executing the submit.
        std::array< VkSemaphore, 1 > signalSemaphores{ renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = signalSemaphores.size();
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        // Reset the fence.
        vkResetFences(vkDevice, 1, &inFlightFences[currentFrame]);

        // Submit to the queue.
        if (vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            std::cerr << "Failed to submit" << std::endl;
            abort();
        }

        // Prepare an image for presentation.
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        // Specify semaphores we need to wait before presenting the image.
        presentInfo.waitSemaphoreCount = signalSemaphores.size();
        presentInfo.pWaitSemaphores = signalSemaphores.data();
        std::array< VkSwapchainKHR, 1 > swapChains{ vkSwapChain };
        presentInfo.swapchainCount = swapChains.size();
        presentInfo.pSwapchains = swapChains.data();
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        // Submit and image for presentaion.
        vkQueuePresentKHR(vkPresentQueue, &presentInfo);

        // Switch to the next frame in the loop.
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // ==========================================================================
    // STEP 5: Deinitialization
    // ==========================================================================
    // Destroy all created Vukan structures in a reverse order.
    // ==========================================================================

    // Wait until all pending render operations are finished.
    vkDeviceWaitIdle(vkDevice);

    // Destroy fences.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(vkDevice, inFlightFences[i], nullptr);
    }

    // Destroy semaphores.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkDevice, renderFinishedSemaphores[i], nullptr);
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkDevice, imageAvailableSemaphores[i], nullptr);
    }

    // Destroy swap uniform buffers.
    for (size_t i = 0; i < vkSwapChainImages.size(); i++) {
        vkDestroyBuffer(vkDevice, uniformBuffers[i], nullptr);
        vkFreeMemory(vkDevice, uniformBuffersMemory[i], nullptr);
    }

    // Destory descriptor pool for uniforms.
    vkDestroyDescriptorPool(vkDevice, descriptorPool, nullptr);

    // Destroy vertex buffer.
    vkDestroyBuffer(vkDevice, vertexBuffer, nullptr);
    vkFreeMemory(vkDevice, vertexBufferMemory, nullptr);

    // Destory command pool
    vkDestroyCommandPool(vkDevice, commandPool, nullptr);

    // Destory framebuffers.
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
    }

    // Destroy depth-stensil image and image view.
    vkDestroyImageView(vkDevice, depthImageView, nullptr);
    vkDestroyImage(vkDevice, depthImage, nullptr);
    vkFreeMemory(vkDevice, depthImageMemory, nullptr);

    // Destory pipeline.
    vkDestroyPipeline(vkDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(vkDevice, renderPass, nullptr);

    // Destroy shader modules.
    vkDestroyShaderModule(vkDevice, vkFragmentShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkVertexShaderModule, nullptr);

    // Destory swap chain image views.
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(vkDevice, imageView, nullptr);
    }

    // Destroy swap chain.
    vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);

    // Destory descriptor set layout for uniforms.
    vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout, nullptr);

    // Destory logical device.
    vkDestroyDevice(vkDevice, nullptr);

#ifdef DEBUG_MODE

    // Get pointer to an extension function vkDestroyDebugUtilsMessengerEXT.
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT == nullptr) {
        std::cerr << "Function vkDestroyDebugUtilsMessengerEXT not found!";
        abort();
    }

    // Destory debug messenger.
    vkDestroyDebugUtilsMessengerEXT(vkInstance, vkDebugMessenger, nullptr);

#endif

    // Destory surface.
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

    // Destroy Vulkan instance.
    vkDestroyInstance(vkInstance, nullptr);

    // Destroy window.
    glfwDestroyWindow(glfwWindow);

    // Deinitialize GLFW library.
    glfwTerminate();

    return 0;
}
