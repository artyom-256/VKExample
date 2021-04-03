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
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.   *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *       The application shows initialization of Vulkan, creation of        *
 *     vertex buffers, shaders and uniform buffers in order to display      *
 *        a simple example of 3D graphics - rotating colored cube.          *
 *                                                                          *
 *    The example is designed to demonstrate pure sequence of actions       *
 *    required to create a Vulkan 3D application, so all code is done       *
 *      as a large main() function and some duplication is present.         *
 *                                                                          *
 *   You may thing about how to split this into modules in order to make    *
 *                    an engine for your application.                       *
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
 * Window width.
 */
constexpr int WINDOW_WIDTH = 800;
/**
 * Window height.
 */
constexpr int WINDOW_HEIGHT = 800;
/**
 * Application name.
 */
constexpr const char* APPLICATION_NAME = "VKExample";
/**
 * Maximal amount of frames processed at the same time.
 */
constexpr int MAX_FRAMES_IN_FLIGHT = 5;

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
    GLFWwindow* glfwWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APPLICATION_NAME, nullptr, nullptr);

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
    //              STEP 3: Select validation layers for debug
    // ==========================================================================
    // Validation layers is a mechanism to hook Vulkan API calls, validate
    // them and notify the user if something goes wrong.
    // Here we need to make sure that requested validation layers are available.
    // ==========================================================================

    // Specify desired validation layers.
    const std::vector< const char* > desiredValidationLayers = {
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
            if (std::strcmp(requestedLayer, availableLayer.layerName) == 0) {
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
    //                STEP 4: Create a debug message callback
    // ==========================================================================
    // Debug message callback allows us to display errors and warnings if
    // we do some mistake using Vulkan API.
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
    //                    STEP 5: Create a Vulkan instance
    // ==========================================================================
    // Vulkan instance is a starting point of using Vulkan API.
    // Here we specify API version and which extensions to use.
    // ==========================================================================

    // Specify application info and required Vulkan version.
    VkApplicationInfo vkAppInfo {};
    // Information about your application.
    vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkAppInfo.pApplicationName = APPLICATION_NAME;
    vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    // Information about your 3D engine (if applicable).
    vkAppInfo.pEngineName = APPLICATION_NAME;
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
        std::cerr << "Failed to creae a Vulkan instance!";
        abort();
    }

    // ==========================================================================
    //                    STEP 6: Create a window surface
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
    //                    STEP 7: Attach a message handler
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
    //                      STEP 8: Pick a physical device
    // ==========================================================================
    // Physical devices correspond to graphical cards available in the system.
    // Before we continue, we should make sure the graphical card is suitable
    // for our needs and in case there is more than one card
    // in the system - select one of them.
    // We are going to perform some tests and fetch device information that
    // we will need after. So, to not request this information twice,
    // we will fill some structures declared below for the selected
    // physical device and use them in the rest of code.
    // ==========================================================================

    // --------------------------------------------------------------------------
    //                    Information about selected device
    // --------------------------------------------------------------------------
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
    VkFormat vkDepthFormat = VK_FORMAT_UNDEFINED;
    // --------------------------------------------------------------------------

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
    std::vector< VkPhysicalDevice > vkDevices(vkDeviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &vkDeviceCount, vkDevices.data());

    // Go through the list of physical device and select the first suitable one.
    // In advanced applications you may introduce a rating to choose
    // the best video card or let the user select one manually.
    for (auto device : vkDevices) {
        // Get properties of the physical device.
        VkPhysicalDeviceProperties vkDeviceProperties;
        vkGetPhysicalDeviceProperties(device, &vkDeviceProperties);

        // Get features of the physical device.
        VkPhysicalDeviceFeatures vkDeviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &vkDeviceFeatures);

        // Get extensions available for the physical device.
        uint32_t vkExtensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &vkExtensionCount, nullptr);
        std::vector< VkExtensionProperties > vkAvailableExtensions(vkExtensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &vkExtensionCount, vkAvailableExtensions.data());

        // ---------------------------------------------------
        // TEST 1: Check if all desired extensions are present
        // ---------------------------------------------------

        // Get list of extensions and compare it to desired one.
        std::set< std::string > requiredExtensions(desiredDeviceExtensions.begin(), desiredDeviceExtensions.end());
        for (const auto& extension : vkAvailableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        bool allExtensionsAvailable = requiredExtensions.empty();

        // ----------------------------------------------------------
        // TEST 2: Check if all required queue families are supported
        // ----------------------------------------------------------

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
            VkBool32 vkPresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vkSurface, &vkPresentSupport);
            if (vkPresentSupport) {
                currentDeviceQueueFamilyIndices.presentFamily = i;
            }
        }
        bool queuesOk = currentDeviceQueueFamilyIndices.graphicsFamily.has_value() &&
                        currentDeviceQueueFamilyIndices.presentFamily.has_value();

        // ---------------------------------------------------------
        // TEST 3: Check if the swap chain supports required formats
        // ---------------------------------------------------------

        // Fill swap chain information into a SwapChainSupportDetails structure.
        SwapChainSupportDetails currenDeviceSwapChainDetails;
        // We should do this only in case the device supports swap buffer.
        // To avoid extra flags and more complex logic, just make sure that all
        // desired extensions have been found.
        if (allExtensionsAvailable) {
            // Get surface capabilities.
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkSurface, &currenDeviceSwapChainDetails.capabilities);
            // Get supported formats.
            uint32_t vkPhysicalDeviceFormatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &vkPhysicalDeviceFormatCount, nullptr);
            if (vkPhysicalDeviceFormatCount != 0) {
                currenDeviceSwapChainDetails.formats.resize(vkPhysicalDeviceFormatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &vkPhysicalDeviceFormatCount, currenDeviceSwapChainDetails.formats.data());
            }
            // Get supported present modes.
            uint32_t vkPhysicalDevicePresentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &vkPhysicalDevicePresentModeCount, nullptr);
            if (vkPhysicalDevicePresentModeCount != 0) {
                currenDeviceSwapChainDetails.presentModes.resize(vkPhysicalDevicePresentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &vkPhysicalDevicePresentModeCount, currenDeviceSwapChainDetails.presentModes.data());
            }
        }
        bool swapChainOk = !currenDeviceSwapChainDetails.formats.empty() &&
                           !currenDeviceSwapChainDetails.presentModes.empty();

        // ----------------------------------------------
        // TEST 4: Check if the depth buffer is avaialble
        // ----------------------------------------------

        // Select a format of depth buffer.
        // We have a list of formats we need to test and pick one.
        std::vector< VkFormat > depthFormatCandidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        VkFormat currentDepthFormat = VK_FORMAT_UNDEFINED;
        for (VkFormat format : depthFormatCandidates) {
            VkFormatProperties vkProps;
            vkGetPhysicalDeviceFormatProperties(device, format, &vkProps);
            if (vkProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                currentDepthFormat = format;
                break;
            }
        }
        bool depthFormatOk = (currentDepthFormat != VK_FORMAT_UNDEFINED);

        // Select the first suitable device.
        if (allExtensionsAvailable && queuesOk && swapChainOk && depthFormatOk) {
            vkPhysicalDevice = device;
            queueFamilyIndices = currentDeviceQueueFamilyIndices;
            swapChainSupportDetails = currenDeviceSwapChainDetails;
            vkDepthFormat = currentDepthFormat;
            break;
        }
    }

    // Check if we have found any suitable device.
    if (vkPhysicalDevice == VK_NULL_HANDLE) {
        std::cerr << "No suitable physical devices available!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                   STEP 9: Create a logical device
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
    VkDeviceCreateInfo vkDeviceCreateInfo {};
    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    vkDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    vkDeviceCreateInfo.pEnabledFeatures = &vkDeviceFeatures;
    // Specify which extensions we want to enable.
    vkDeviceCreateInfo.enabledExtensionCount = static_cast< uint32_t >(desiredDeviceExtensions.size());
    vkDeviceCreateInfo.ppEnabledExtensionNames = desiredDeviceExtensions.data();

#ifdef DEBUG_MODE

    // Switch on all requested layers for debug mode.
    vkDeviceCreateInfo.enabledLayerCount = static_cast< uint32_t >(desiredValidationLayers.size());
    vkDeviceCreateInfo.ppEnabledLayerNames = desiredValidationLayers.data();

#else

    vkDeviceCreateInfo.enabledLayerCount = 0;

#endif

    // Create a logical device.
    if (vkCreateDevice(vkPhysicalDevice, &vkDeviceCreateInfo, nullptr, &vkDevice) != VK_SUCCESS) {
        std::cerr << "Failed to create a logical device!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                   STEP 10: Select surface configuration
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
    //                     STEP 11: Create a swap chain
    // ==========================================================================
    // Swap chain is a chain of rendered images that are going to be displayed
    // on the screen. It is used to synchronize image rendering with refresh
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
    //               STEP 12: Create a descriptor set layout
    // ==========================================================================
    // Descriptor set layout describes details of every uniform data binding
    // using in shaders. This is needed if we want to use uniforms in shaders.
    // ==========================================================================

    VkDescriptorSetLayoutBinding vkUboLayoutBinding{};
    vkUboLayoutBinding.binding = 0;
    vkUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vkUboLayoutBinding.descriptorCount = 1;
    vkUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vkUboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayout vkDescriptorSetLayout;

    VkDescriptorSetLayoutCreateInfo vkLayoutInfo{};
    vkLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    vkLayoutInfo.bindingCount = 1;
    vkLayoutInfo.pBindings = &vkUboLayoutBinding;

    if (vkCreateDescriptorSetLayout(vkDevice, &vkLayoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create a descriptor set layout" << std::endl;
        abort();
    }

    // ==========================================================================
    //                        STEP 13: Load shaders
    // ==========================================================================
    // Shaders are special code that is executed directly on GPU.
    // For our simple example we use vertex and fragment shaders.
    // Unlike OpenGL, Vulcan uses a binary format which is called SPIR-V and
    // each shader should be compiled to this format using glslc compiler that
    // could be found in Vulkan SDK.
    // ==========================================================================

    // --------------------------------------------------------------------------
    // Create a vertex shader module.
    // --------------------------------------------------------------------------

    // Open file.
    std::ifstream vertexShaderFile("main.vert.spv", std::ios::ate | std::ios::binary);
    if (!vertexShaderFile.is_open()) {
        std::cerr << "Vertex shader file not found!" << std::endl;
        abort();
    }
    // Calculate file size.
    size_t vertexFileSize = static_cast< size_t >(vertexShaderFile.tellg());
    // Jump to the beginning of the file.
    vertexShaderFile.seekg(0);
    // Read shader code.
    std::vector< char > vertexShaderBuffer(vertexFileSize);
    vertexShaderFile.read(vertexShaderBuffer.data(), vertexFileSize);
    // Close the file.
    vertexShaderFile.close();
    // Shader module creation info.
    VkShaderModuleCreateInfo vkVertexShaderCreateInfo{};
    vkVertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkVertexShaderCreateInfo.codeSize = vertexShaderBuffer.size();
    vkVertexShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(vertexShaderBuffer.data());
    // Create a vertex shader module.
    VkShaderModule vkVertexShaderModule;
    if (vkCreateShaderModule(vkDevice, &vkVertexShaderCreateInfo, nullptr, &vkVertexShaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create a shader!" << std::endl;
        abort();
    }

    // --------------------------------------------------------------------------
    // Create a fragment shader module.
    // --------------------------------------------------------------------------

    // Open file.
    std::ifstream fragmentShaderFile("main.frag.spv", std::ios::ate | std::ios::binary);
    if (!fragmentShaderFile.is_open()) {
        std::cerr << "Fragment shader file not found!" << std::endl;
        abort();
    }
    // Calculate file size.
    size_t fragmentFileSize = static_cast< size_t >(fragmentShaderFile.tellg());
    // Jump to the beginning of the file.
    fragmentShaderFile.seekg(0);
    // Read shader code.
    std::vector< char > fragmentShaderBuffer(fragmentFileSize);
    fragmentShaderFile.read(fragmentShaderBuffer.data(), fragmentFileSize);
    // Close the file.
    fragmentShaderFile.close();
    // Shader module creation info.
    VkShaderModuleCreateInfo vkFragmentShaderCreateInfo{};
    vkFragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkFragmentShaderCreateInfo.codeSize = fragmentShaderBuffer.size();
    vkFragmentShaderCreateInfo.pCode = reinterpret_cast< const uint32_t* >(fragmentShaderBuffer.data());
    // Create a fragment shader module.
    VkShaderModule vkFragmentShaderModule;
    if (vkCreateShaderModule(vkDevice, &vkFragmentShaderCreateInfo, nullptr, &vkFragmentShaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create a shader!" << std::endl;
        abort();
    }

    // --------------------------------------------------------------------------

    // Create a pipeline stage for a vertex shader.
    VkPipelineShaderStageCreateInfo vkVertShaderStageInfo{};
    vkVertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkVertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vkVertShaderStageInfo.module = vkVertexShaderModule;
    // main function name
    vkVertShaderStageInfo.pName = "main";

    // Create a pipeline stage for a fragment shader.
    VkPipelineShaderStageCreateInfo vkFragShaderStageInfo{};
    vkFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    vkFragShaderStageInfo.module = vkFragmentShaderModule;
    // main function name
    vkFragShaderStageInfo.pName = "main";

    // Put both shader modules into an array.
    std::array< VkPipelineShaderStageCreateInfo, 2 > shaderStages {
        vkVertShaderStageInfo,
        vkFragShaderStageInfo
    };

    // ==========================================================================
    //                    STEP 14: Create a vertex buffer
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

    // Binding descriptor specifies how our array is splited into vertices.
    // In particular, we say that each sizeof(Vertex) bytes correspond to one vertex.
    VkVertexInputBindingDescription vkBindingDescription{};
    vkBindingDescription.binding = 0;
    vkBindingDescription.stride = sizeof(Vertex);
    vkBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Attribute description specifies how one vertext is split into separate variables.
    // In our case a vertext is a composition of two vec3 values: coordinates and color.
    // Property location corresponds to a location value in shader code.
    std::array< VkVertexInputAttributeDescription, 2 > vkAttributeDescriptions{};
    // Description of the first attribute (coordinates).
    vkAttributeDescriptions[0].binding = 0;
    vkAttributeDescriptions[0].location = 0;
    vkAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vkAttributeDescriptions[0].offset = offsetof(Vertex, pos);
    // Description of the second attribute (color).
    vkAttributeDescriptions[1].binding = 0;
    vkAttributeDescriptions[1].location = 1;
    vkAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vkAttributeDescriptions[1].offset = offsetof(Vertex, color);

    // Create a vertex input state for pipeline creation.
    VkPipelineVertexInputStateCreateInfo vkVertexInputInfo{};
    vkVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vkVertexInputInfo.vertexBindingDescriptionCount = 1;
    vkVertexInputInfo.pVertexBindingDescriptions = &vkBindingDescription;
    vkVertexInputInfo.vertexAttributeDescriptionCount = static_cast< uint32_t >(vkAttributeDescriptions.size());
    vkVertexInputInfo.pVertexAttributeDescriptions = vkAttributeDescriptions.data();

    // ==========================================================================
    //               STEP 15: Create a pipeline assembly state
    // ==========================================================================
    // Pipeline assembly state describes a geometry of the input data.
    // In our case the input is a list of triangles.
    // ==========================================================================

    VkPipelineInputAssemblyStateCreateInfo vkInputAssembly{};
    vkInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vkInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vkInputAssembly.primitiveRestartEnable = VK_FALSE;

    // ==========================================================================
    //                 STEP 16: Create a viewport and scissors
    // ==========================================================================
    // Viewport is a region of a framebuffer that will be used for renderring.
    // Scissors define if some part of rendered image should be cut.
    // In our example we define both viewport and scissors equal to
    // the framebuffer size.
    // ==========================================================================

    // Create a viewport.
    VkViewport vkViewport{};
    vkViewport.x = 0.0f;
    vkViewport.y = 0.0f;
    vkViewport.width = static_cast< float >(vkSelectedExtent.width);
    vkViewport.height = static_cast< float >(vkSelectedExtent.height);
    vkViewport.minDepth = 0.0f;
    vkViewport.maxDepth = 1.0f;

    // Create scissors.
    VkRect2D vkScissor{};
    vkScissor.offset = {0, 0};
    vkScissor.extent = vkSelectedExtent;

    // Make a structure for framebuffer creation.
    VkPipelineViewportStateCreateInfo vkViewportState{};
    vkViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vkViewportState.viewportCount = 1;
    vkViewportState.pViewports = &vkViewport;
    vkViewportState.scissorCount = 1;
    vkViewportState.pScissors = &vkScissor;

    // ==========================================================================
    //                 STEP 17: Create a rasterization stage
    // ==========================================================================
    // Rasterization stage takes primitives and rasterizes them to fragments
    // pased to the fragment shader.
    // ==========================================================================

    // Rasterizer create info
    VkPipelineRasterizationStateCreateInfo vkRasterizer{};
    vkRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    vkRasterizer.depthClampEnable = VK_FALSE;
    vkRasterizer.rasterizerDiscardEnable = VK_FALSE;
    // Fill in triangles.
    vkRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    vkRasterizer.lineWidth = 1.0f;
    // Enable face culling.
    vkRasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    vkRasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    vkRasterizer.depthBiasEnable = VK_FALSE;
    vkRasterizer.depthBiasConstantFactor = 0.0f;
    vkRasterizer.depthBiasClamp = 0.0f;
    vkRasterizer.depthBiasSlopeFactor = 0.0f;

    // ==========================================================================
    //                     STEP 18: Create an MSAA state
    // ==========================================================================
    // MultiSample Anti-Aliasing is used to make edges smoother by rendering
    // them in higher resolution (having more then one fragment per pixel).
    // ==========================================================================

    // Select the maximal amount of samples supported by the device.
    VkSampleCountFlagBits vkMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
    vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkPhysicalDeviceProperties);
    VkSampleCountFlags vkSampleCounts = vkPhysicalDeviceProperties.limits.framebufferColorSampleCounts & vkPhysicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (vkSampleCounts & VK_SAMPLE_COUNT_64_BIT) {
        vkMsaaSamples = VK_SAMPLE_COUNT_64_BIT;
    } else if (vkSampleCounts & VK_SAMPLE_COUNT_32_BIT) {
        vkMsaaSamples = VK_SAMPLE_COUNT_32_BIT;
    } else if (vkSampleCounts & VK_SAMPLE_COUNT_16_BIT) {
        vkMsaaSamples = VK_SAMPLE_COUNT_16_BIT;
    } else if (vkSampleCounts & VK_SAMPLE_COUNT_8_BIT) {
        vkMsaaSamples = VK_SAMPLE_COUNT_8_BIT;
    } else if (vkSampleCounts & VK_SAMPLE_COUNT_4_BIT) {
        vkMsaaSamples = VK_SAMPLE_COUNT_4_BIT;
    } else if (vkSampleCounts & VK_SAMPLE_COUNT_2_BIT) {
        vkMsaaSamples = VK_SAMPLE_COUNT_2_BIT;
    }

    // Create a state for MSAA.
    VkPipelineMultisampleStateCreateInfo vkMultisampling{};
    vkMultisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    vkMultisampling.sampleShadingEnable = VK_FALSE;
    vkMultisampling.rasterizationSamples = vkMsaaSamples;
    vkMultisampling.minSampleShading = 1.0f;
    vkMultisampling.pSampleMask = nullptr;
    vkMultisampling.alphaToCoverageEnable = VK_FALSE;
    vkMultisampling.alphaToOneEnable = VK_FALSE;

    // ==========================================================================
    //                  STEP 19: Create a resolve attachment
    // ==========================================================================
    // In order to implement MSAA we should introduce a so-called
    // resolve attachment. The attachment has only 1 sample and while rendering
    // a multi-sampled image will be resolved to this attachment.
    // To make it work we shoukld specify the resolve attachment
    // during subpass creation.
    // ==========================================================================

    // Description of an image for resolve attachment.
    VkImageCreateInfo vkColorImageInfo{};
    vkColorImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkColorImageInfo.imageType = VK_IMAGE_TYPE_2D;
    vkColorImageInfo.extent.width = vkSelectedExtent.width;
    vkColorImageInfo.extent.height = vkSelectedExtent.height;
    vkColorImageInfo.extent.depth = 1;
    vkColorImageInfo.mipLevels = 1;
    vkColorImageInfo.arrayLayers = 1;
    vkColorImageInfo.format = vkSelectedFormat.format;
    vkColorImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkColorImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkColorImageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vkColorImageInfo.samples = vkMsaaSamples;
    vkColorImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create an image for resolve attachment.
    VkImage colorImage;
    if (vkCreateImage(vkDevice, &vkColorImageInfo, nullptr, &colorImage) != VK_SUCCESS) {
        std::cerr << "Failed to create an image!" << std::endl;
        abort();
    }

    // Get memory requirements.
    VkMemoryRequirements vkMemRequirements;
    vkGetImageMemoryRequirements(vkDevice, colorImage, &vkMemRequirements);

    // Description of memory allocation.
    VkMemoryAllocateInfo vkColorImageAllocInfo{};
    vkColorImageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkColorImageAllocInfo.allocationSize = vkMemRequirements.size;
    // Find memory type.
    VkPhysicalDeviceMemoryProperties vkColorImageMemProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkColorImageMemProperties);
    uint32_t colorImageMemoryTypeInex = UINT32_MAX;
    for (uint32_t i = 0; i < vkColorImageMemProperties.memoryTypeCount; i++) {
        if ((vkMemRequirements.memoryTypeBits & (1 << i)) && (vkColorImageMemProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            colorImageMemoryTypeInex = i;
            break;
        }
    }
    vkColorImageAllocInfo.memoryTypeIndex = colorImageMemoryTypeInex;

    // Allocate memory for resolve attachment.
    VkDeviceMemory colorImageMemory;
    if (vkAllocateMemory(vkDevice, &vkColorImageAllocInfo, nullptr, &colorImageMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate image memory!" << std::endl;
        abort();
    }

    // Bind the image to the memory.
    vkBindImageMemory(vkDevice, colorImage, colorImageMemory, 0);

    // Describe an image view for resolve attachment.
    VkImageViewCreateInfo vkColorImageViewInfo{};
    vkColorImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vkColorImageViewInfo.image = colorImage;
    vkColorImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vkColorImageViewInfo.format = vkSelectedFormat.format;
    vkColorImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkColorImageViewInfo.subresourceRange.baseMipLevel = 0;
    vkColorImageViewInfo.subresourceRange.levelCount = 1;
    vkColorImageViewInfo.subresourceRange.baseArrayLayer = 0;
    vkColorImageViewInfo.subresourceRange.layerCount = 1;

    // Create an image view for resolve attachment.
    VkImageView colorImageView;
    if (vkCreateImageView(vkDevice, &vkColorImageViewInfo, nullptr, &colorImageView) != VK_SUCCESS) {
        std::cerr << "Failed to create texture image view!" << std::endl;
        abort();
    }

    // Describe a resolve attachment.
    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = vkSelectedFormat.format;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Resolve attachment reference.
    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // ==========================================================================
    //               STEP 20: Configure depth and stensil tests
    // ==========================================================================
    // Depth and stensil attachment is created and now we need to configure
    // these tests. In this example we use regular VK_COMPARE_OP_LESS depth
    // operation and disable stensil test.
    // ==========================================================================

    VkPipelineDepthStencilStateCreateInfo vkDepthStencil{};
    vkDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    vkDepthStencil.depthTestEnable = VK_TRUE;
    vkDepthStencil.depthWriteEnable = VK_TRUE;
    vkDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    vkDepthStencil.depthBoundsTestEnable = VK_FALSE;
    vkDepthStencil.minDepthBounds = 0.0f;
    vkDepthStencil.maxDepthBounds = 1.0f;
    vkDepthStencil.stencilTestEnable = VK_FALSE;
    vkDepthStencil.front = VkStencilOpState{};
    vkDepthStencil.back = VkStencilOpState{};

    // ==========================================================================
    //                 STEP 21: Create a color blend state
    // ==========================================================================
    // Color blend state describes how fragments are applied to the result
    // image. There might be options like mixing, but we switch off blending and
    // simply put a new color instead of existing one.
    // ==========================================================================

    // Configuration per attached framebuffer.
    VkPipelineColorBlendAttachmentState vkColorBlendAttachment{};
    vkColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    vkColorBlendAttachment.blendEnable = VK_FALSE;
    // Other fields are optional.
    vkColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    vkColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    vkColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    vkColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    vkColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    vkColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // Global color blending settings.
    VkPipelineColorBlendStateCreateInfo vkColorBlending{};
    vkColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    vkColorBlending.attachmentCount = 1;
    vkColorBlending.pAttachments = &vkColorBlendAttachment;
    vkColorBlending.logicOpEnable = VK_FALSE;
    // Other fields are optional.
    vkColorBlending.logicOp = VK_LOGIC_OP_COPY;
    vkColorBlending.blendConstants[0] = 0.0f;
    vkColorBlending.blendConstants[1] = 0.0f;
    vkColorBlending.blendConstants[2] = 0.0f;
    vkColorBlending.blendConstants[3] = 0.0f;

    // ==========================================================================
    //                   STEP 22: Create a color attachment
    // ==========================================================================
    // Color attachment contains bytes of rendered image, so we should create
    // one in order to display something.
    // ==========================================================================

    // Descriptor of a color attachment.
    VkAttachmentDescription vkColorAttachment{};
    vkColorAttachment.format = vkSelectedFormat.format;
    vkColorAttachment.samples = vkMsaaSamples;
    vkColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    vkColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    vkColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vkColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vkColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Since we use MSAA, we should specify VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL here.
    // If we disable MSAA, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR will be enough.
    vkColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Color attachment reference.
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // ==========================================================================
    //            STEP 23: Create a depth and stensil attachment
    // ==========================================================================
    // Depth and stensil attachment is used to support two tests:
    // - depth test - Makes sure that only the nearest fragment is displayed.
    // - stensil test - Allows to cut some fragments depending on values of
    //                  the stensil buffer.
    // Although in this example we only need a depth test, both of them use
    // the same attachment.
    // ==========================================================================

    // Descriptor of a depth attachment.
    VkAttachmentDescription vkDepthAttachment{};
    vkDepthAttachment.format = vkDepthFormat;
    vkDepthAttachment.samples = vkMsaaSamples;
    vkDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    vkDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vkDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vkDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vkDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkDepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Depth attachment reference.
    VkAttachmentReference vkDepthAttachmentRef{};
    vkDepthAttachmentRef.attachment = 1;
    vkDepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // ==========================================================================
    //                     STEP 24: Create a render pass
    // ==========================================================================
    // Render pass represents a collection of attachments, subpasses
    // and dependencies between the subpasses.
    // Subpasses allow to organize rendering process as a chain of operations.
    // Each operation is applied to the result of the previous one.
    // In the example we only need a single subpass.
    // ==========================================================================

    // Define a subpass and include both attachments (color and depth-stensil).
    VkSubpassDescription vkSubpass{};
    vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkSubpass.colorAttachmentCount = 1;
    vkSubpass.pColorAttachments = &colorAttachmentRef;
    vkSubpass.pDepthStencilAttachment = &vkDepthAttachmentRef;
    vkSubpass.pResolveAttachments = &colorAttachmentResolveRef;

    // Define a subpass dependency.
    VkSubpassDependency vkDependency{};
    vkDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    vkDependency.dstSubpass = 0;
    vkDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    vkDependency.srcAccessMask = 0;
    vkDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    vkDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Define a render pass and attach the subpass.
    VkRenderPassCreateInfo vkRenderPassInfo{};
    vkRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    std::array< VkAttachmentDescription, 3 > attachments = { vkColorAttachment, vkDepthAttachment, colorAttachmentResolve };
    vkRenderPassInfo.attachmentCount = static_cast< uint32_t >(attachments.size());
    vkRenderPassInfo.pAttachments = attachments.data();
    vkRenderPassInfo.subpassCount = 1;
    vkRenderPassInfo.pSubpasses = &vkSubpass;
    vkRenderPassInfo.dependencyCount = 1;
    vkRenderPassInfo.pDependencies = &vkDependency;

    // Create a render pass.
    VkRenderPass vkRenderPass;
    if (vkCreateRenderPass(vkDevice, &vkRenderPassInfo, nullptr, &vkRenderPass) != VK_SUCCESS) {
        std::cerr << "Failed to create a render pass!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                   STEP 25: Create a graphics pipeline
    // ==========================================================================
    // All stages prepared above should be combined into a graphics pipeline.
    // ==========================================================================

    // Define a pipeline layout.
    VkPipelineLayoutCreateInfo vkPipelineLayoutInfo{};
    vkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkPipelineLayoutInfo.setLayoutCount = 1;
    vkPipelineLayoutInfo.pSetLayouts = &vkDescriptorSetLayout;
    vkPipelineLayoutInfo.pushConstantRangeCount = 0;
    vkPipelineLayoutInfo.pPushConstantRanges = nullptr;

    // Create a pipeline layout.
    VkPipelineLayout vkPipelineLayout;
    if (vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to creare a pipeline layout!" << std::endl;
        abort();
    }

    // Define a pipeline and provide all stages created above.
    VkGraphicsPipelineCreateInfo vkPipelineInfo{};
    vkPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    vkPipelineInfo.stageCount = shaderStages.size();
    vkPipelineInfo.pStages = shaderStages.data();
    vkPipelineInfo.pVertexInputState = &vkVertexInputInfo;
    vkPipelineInfo.pInputAssemblyState = &vkInputAssembly;
    vkPipelineInfo.pViewportState = &vkViewportState;
    vkPipelineInfo.pRasterizationState = &vkRasterizer;
    vkPipelineInfo.pMultisampleState = &vkMultisampling;
    vkPipelineInfo.pDepthStencilState = &vkDepthStencil;
    vkPipelineInfo.pColorBlendState = &vkColorBlending;
    vkPipelineInfo.pDynamicState = nullptr;
    vkPipelineInfo.layout = vkPipelineLayout;
    vkPipelineInfo.renderPass = vkRenderPass;
    vkPipelineInfo.subpass = 0;
    vkPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    vkPipelineInfo.basePipelineIndex = -1;

    // Create a pipeline.
    VkPipeline vkGraphicsPipeline;
    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &vkPipelineInfo, nullptr, &vkGraphicsPipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create a graphics pipeline!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                 STEP 26: Create swap chain image views
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
    std::vector< VkImageView > vkSwapChainImageViews;
    vkSwapChainImageViews.resize(vkSwapChainImageCount);
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
        if (vkCreateImageView(vkDevice, &createInfo, nullptr, &vkSwapChainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create an image view #" << i << "!" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    //                  STEP 27: Create a depth buffer image
    // ==========================================================================
    // In order to use a depth buffer, we should create an image.
    // Unlike swap buffer images, we need only one depth image and it should
    // be created explicitly.
    // ==========================================================================

    // Describe a depth image.
    VkImageCreateInfo vkImageInfo{};
    vkImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    vkImageInfo.imageType = VK_IMAGE_TYPE_2D;
    vkImageInfo.extent.width = vkSelectedExtent.width;
    vkImageInfo.extent.height = vkSelectedExtent.height;
    vkImageInfo.extent.depth = 1;
    vkImageInfo.mipLevels = 1;
    vkImageInfo.arrayLayers = 1;
    vkImageInfo.format = vkDepthFormat;
    vkImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    vkImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    vkImageInfo.samples = vkMsaaSamples;
    vkImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a depth image.
    VkImage vkDepthImage;
    if (vkCreateImage(vkDevice, &vkImageInfo, nullptr, &vkDepthImage) != VK_SUCCESS) {
        std::cerr << "Failed to create a depth image!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the depth image.
    VkMemoryRequirements vkmDepthMemRequirements;
    vkGetImageMemoryRequirements(vkDevice, vkDepthImage, &vkmDepthMemRequirements);

    // Define memory allocate info.
    VkMemoryAllocateInfo memoryAllocInfo{};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = vkmDepthMemRequirements.size;
    // Find a suitable memory type.
    uint32_t memTypeIndex = UINT32_MAX;
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((vkmDepthMemRequirements.memoryTypeBits & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memTypeIndex = i;
            break;
        }
    }
    memoryAllocInfo.memoryTypeIndex = memTypeIndex;

    // Allocate memory for the depth image.
    VkDeviceMemory vkDepthImageMemory;
    if (vkAllocateMemory(vkDevice, &memoryAllocInfo, nullptr, &vkDepthImageMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate image memory!" << std::endl;
        abort();
    }

    // Bind the image to the allocated memory.
    vkBindImageMemory(vkDevice, vkDepthImage, vkDepthImageMemory, 0);

    // ==========================================================================
    //                STEP 28: Create a depth buffer image view
    // ==========================================================================
    // Similarly to swap buffer images, we need an image view to use it.
    // ==========================================================================

    // Destribe an image view.
    VkImageViewCreateInfo vkViewInfo{};
    vkViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vkViewInfo.image = vkDepthImage;
    vkViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vkViewInfo.format = vkDepthFormat;
    vkViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    vkViewInfo.subresourceRange.baseMipLevel = 0;
    vkViewInfo.subresourceRange.levelCount = 1;
    vkViewInfo.subresourceRange.baseArrayLayer = 0;
    vkViewInfo.subresourceRange.layerCount = 1;

    // Create an image view.
    VkImageView vkDepthImageView;
    if (vkCreateImageView(vkDevice, &vkViewInfo, nullptr, &vkDepthImageView) != VK_SUCCESS) {
        std::cerr << "Failed to create a texture image view!" << std::endl;
        abort();
    }

    // ==========================================================================
    //                     STEP 29: Create framebuffers
    // ==========================================================================
    // Framebuffer refers to all attachments that are output of the rendering
    // process.
    // ==========================================================================

    // Create framebuffers.
    std::vector< VkFramebuffer > vkSwapChainFramebuffers;
    vkSwapChainFramebuffers.resize(vkSwapChainImageViews.size());
    for (size_t i = 0; i < vkSwapChainImageViews.size(); i++) {
        // We have only two attachments: color and depth.
        // Depth attachment is shared.
        std::array< VkImageView, 3 > attachments = {
            colorImageView,
            vkDepthImageView,
            vkSwapChainImageViews[i]
        };

        // Describe a framebuffer.
        VkFramebufferCreateInfo vkFramebufferInfo{};
        vkFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        vkFramebufferInfo.renderPass = vkRenderPass;
        vkFramebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
        vkFramebufferInfo.pAttachments =  attachments.data();
        vkFramebufferInfo.width = vkSelectedExtent.width;
        vkFramebufferInfo.height = vkSelectedExtent.height;
        vkFramebufferInfo.layers = 1;

        // Create a framebuffer.
        if (vkCreateFramebuffer(vkDevice, &vkFramebufferInfo, nullptr, &vkSwapChainFramebuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a framebuffer!" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    //                    STEP 30: Create a vertex buffer
    // ==========================================================================
    // Vertex buffer contains vertices of our model we want to pass
    // to the vertex shader.
    // ==========================================================================

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

    // Calculate buffer size.
    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

    // Describe a buffer.
    VkBufferCreateInfo vkVertexBufferInfo{};
    vkVertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkVertexBufferInfo.size = vertexBufferSize;
    vkVertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkVertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Create a buffer.
    VkBuffer vkVertexBuffer;
    if (vkCreateBuffer(vkDevice, &vkVertexBufferInfo, nullptr, &vkVertexBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to create a vertex buffer!" << std::endl;
        abort();
    }

    // Retrieve memory requirements for the vertex buffer.
    VkMemoryRequirements vkVertexBufferMemRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkVertexBuffer, &vkVertexBufferMemRequirements);

    // Define memory allocate info.
    VkMemoryAllocateInfo vkVertexBufferAllocInfo{};
    vkVertexBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkVertexBufferAllocInfo.allocationSize = vkVertexBufferMemRequirements.size;
    // Find a suitable memory type.
    uint32_t bufferMemTypeIndex = UINT32_MAX;
    VkMemoryPropertyFlags vkBufferMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkPhysicalDeviceMemoryProperties vkBufferMemProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkBufferMemProperties);
    for (uint32_t i = 0; i < vkBufferMemProperties.memoryTypeCount; i++) {
        if ((vkVertexBufferMemRequirements.memoryTypeBits & (1 << i)) &&
                (vkBufferMemProperties.memoryTypes[i].propertyFlags & vkBufferMemFlags) == vkBufferMemFlags) {
            bufferMemTypeIndex = i;
            break;
        }
    }
    vkVertexBufferAllocInfo.memoryTypeIndex = bufferMemTypeIndex;
    // Allocate memory for the vertex buffer.
    VkDeviceMemory vkVertexBufferMemory;
    if (vkAllocateMemory(vkDevice, &vkVertexBufferAllocInfo, nullptr, &vkVertexBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate memroy for the vertex buffer!" << std::endl;
        abort();
    }

    // Bind the buffer to the allocated memory.
    vkBindBufferMemory(vkDevice, vkVertexBuffer, vkVertexBufferMemory, 0);

    // Copy our vertices to the allocated memory.
    void* vertexBufferMemoryData;
    vkMapMemory(vkDevice, vkVertexBufferMemory, 0, vertexBufferSize, 0, &vertexBufferMemoryData);
    memcpy(vertexBufferMemoryData, vertices.data(), (size_t) vertexBufferSize);
    vkUnmapMemory(vkDevice, vkVertexBufferMemory);

    // ==========================================================================
    //                      STEP 31: Create uniform buffers
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
    std::vector< VkBuffer > vkUniformBuffers;
    vkUniformBuffers.resize(vkSwapChainImages.size());

    // Memory of uniform buffers.
    std::vector< VkDeviceMemory > vkUniformBuffersMemory;
    vkUniformBuffersMemory.resize(vkSwapChainImages.size());

    // Create one uniform buffer per swap chain image.
    for (size_t i = 0; i < vkSwapChainImages.size(); i++) {
        // Describe a buffer.
        VkBufferCreateInfo vkBufferInfo{};
        vkBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkBufferInfo.size = bufferSize;
        vkBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        vkBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Create a buffer.
        if (vkCreateBuffer(vkDevice, &vkBufferInfo, nullptr, &vkUniformBuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a buffer!" << std::endl;
            abort();
        }

        // Retrieve memory requirements for the vertex buffer.
        VkMemoryRequirements vkMemRequirements;
        vkGetBufferMemoryRequirements(vkDevice, vkUniformBuffers[i], &vkMemRequirements);

        // Define memory allocate info.
        VkMemoryAllocateInfo vkAllocInfo{};
        vkAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkAllocInfo.allocationSize = vkMemRequirements.size;
        // Select suitable memory type.
        uint32_t memTypeIndex = UINT32_MAX;
        VkMemoryPropertyFlags vkMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VkPhysicalDeviceMemoryProperties vkMemProperties;
        vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkMemProperties);
        for (uint32_t i = 0; i < vkMemProperties.memoryTypeCount; i++) {
            if ((vkMemRequirements.memoryTypeBits & (1 << i)) && (vkMemProperties.memoryTypes[i].propertyFlags & vkMemFlags) == vkMemFlags) {
                memTypeIndex = i;
                break;
            }
        }
        vkAllocInfo.memoryTypeIndex = memTypeIndex;

        // Allocate memory for the vertex buffer.
        if (vkAllocateMemory(vkDevice, &vkAllocInfo, nullptr, &vkUniformBuffersMemory[i]) != VK_SUCCESS) {
            std::cerr << "Failed to allocate buffer memory!" << std::endl;
            abort();
        }

        // Bind the buffer to the allocated memory.
        vkBindBufferMemory(vkDevice, vkUniformBuffers[i], vkUniformBuffersMemory[i], 0);
    }

    // ==========================================================================
    //                  STEP 32: Create descriptopr sets
    // ==========================================================================
    // In order to use uniforms, we should create a descriptor set for each
    // uniform buffer. Descriptors are allocated from the descriptor poll,
    // so we should create it first.
    // ==========================================================================

    // Define a descriptor pool size. We need one descriptor set per swap chain image.
    VkDescriptorPoolSize vkPoolSize{};
    vkPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vkPoolSize.descriptorCount = static_cast< uint32_t >(vkSwapChainImages.size());

    // Define descriptor pool.
    VkDescriptorPoolCreateInfo vkDescriptorPoolInfo{};
    vkDescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    vkDescriptorPoolInfo.poolSizeCount = 1;
    vkDescriptorPoolInfo.pPoolSizes = &vkPoolSize;
    vkDescriptorPoolInfo.maxSets = static_cast< uint32_t >(vkSwapChainImages.size());

    // Create descriptor pool.
    VkDescriptorPool vkDescriptorPool;
    if (vkCreateDescriptorPool(vkDevice, &vkDescriptorPoolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
        std::cerr << "Failed to create a descriptor pool!" << std::endl;
        abort();
    }

    // Take a descriptor set layout created above and use it for all descriptor sets.
    std::vector< VkDescriptorSetLayout > layouts(vkSwapChainImages.size(), vkDescriptorSetLayout);

    // Describe allocate infor for descriptor set.
    VkDescriptorSetAllocateInfo vkDescriptSetAllocInfo{};
    vkDescriptSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vkDescriptSetAllocInfo.descriptorPool = vkDescriptorPool;
    vkDescriptSetAllocInfo.descriptorSetCount = static_cast< uint32_t >(vkSwapChainImages.size());
    vkDescriptSetAllocInfo.pSetLayouts = layouts.data();

    // Create a descriptor set.
    std::vector< VkDescriptorSet > vkDescriptorSets;
    vkDescriptorSets.resize(vkSwapChainImages.size());
    if (vkAllocateDescriptorSets(vkDevice, &vkDescriptSetAllocInfo, vkDescriptorSets.data()) != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor set!" << std::endl;
        abort();
    }

    // Write descriptors for each uniform buffer.
    for (size_t i = 0; i < vkSwapChainImages.size(); i++) {
        // Describe a uniform buffer info.
        VkDescriptorBufferInfo vkBufferInfo{};
        vkBufferInfo.buffer = vkUniformBuffers[i];
        vkBufferInfo.offset = 0;
        vkBufferInfo.range = sizeof(UniformBufferObject);

        // Describe a descriptor set to write.
        VkWriteDescriptorSet vkDescriptorWrite{};
        vkDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkDescriptorWrite.dstSet = vkDescriptorSets[i];
        vkDescriptorWrite.dstBinding = 0;
        vkDescriptorWrite.dstArrayElement = 0;
        vkDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        vkDescriptorWrite.descriptorCount = 1;
        vkDescriptorWrite.pBufferInfo = &vkBufferInfo;
        vkDescriptorWrite.pImageInfo = nullptr;
        vkDescriptorWrite.pTexelBufferView = nullptr;

        // Write the descriptor set.
        vkUpdateDescriptorSets(vkDevice, 1, &vkDescriptorWrite, 0, nullptr);
    }

    // ==========================================================================
    //                    STEP 33: Create command buffers
    // ==========================================================================
    // Command buffers describe a set of rendering commands submitted to Vulkan.
    // We need to have one buffer per each image in the swap chain.
    // Command buffers are taken from the command pool, so we should
    // create one.
    // ==========================================================================

    // Describe a command pool.
    VkCommandPoolCreateInfo vkPoolInfo{};
    vkPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vkPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    vkPoolInfo.flags = 0;

    // Create a command pool.
    VkCommandPool vkCommandPool;
    if (vkCreateCommandPool(vkDevice, &vkPoolInfo, nullptr, &vkCommandPool) != VK_SUCCESS) {
        std::cerr << "Failed to create a command pool!" << std::endl;
        abort();
    }

    // Create a vector for all command buffers.
    std::vector< VkCommandBuffer > vkCommandBuffers;
    vkCommandBuffers.resize(vkSwapChainFramebuffers.size());

    // Describe a command buffer allocate info.
    VkCommandBufferAllocateInfo vkAllocInfo{};
    vkAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkAllocInfo.commandPool = vkCommandPool;
    vkAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocInfo.commandBufferCount = static_cast< uint32_t >(vkCommandBuffers.size());

    // Allocate command buffers.
    if (vkAllocateCommandBuffers(vkDevice, &vkAllocInfo, vkCommandBuffers.data()) != VK_SUCCESS) {
        std::cerr << "Failed to create command buffers" << std::endl;
        abort();
    }

    // Describe a rendering sequence for each command buffer.
    for (size_t i = 0; i < vkCommandBuffers.size(); i++) {
        // Start adding commands into the buffer.
        VkCommandBufferBeginInfo vkBeginInfo{};
        vkBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginInfo.flags = 0;
        vkBeginInfo.pInheritanceInfo = nullptr;
        if (vkBeginCommandBuffer(vkCommandBuffers[i], &vkBeginInfo) != VK_SUCCESS) {
            std::cerr << "Failed to start command buffer recording" << std::endl;
            abort();
        }

        // Define default values of color and depth buffer attachment elements.
        // In our case this means a black color of the background and a maximal depth of each fragment.
        std::array< VkClearValue, 2 > vkClearValues{};
        vkClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        vkClearValues[1].depthStencil = { 1.0f, 0 };

        // Describe a render pass.
        VkRenderPassBeginInfo vkRenderPassBeginInfo{};
        vkRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        vkRenderPassBeginInfo.renderPass = vkRenderPass;
        vkRenderPassBeginInfo.framebuffer = vkSwapChainFramebuffers[i];
        vkRenderPassBeginInfo.renderArea.offset = { 0, 0 };
        vkRenderPassBeginInfo.renderArea.extent = vkSelectedExtent;
        vkRenderPassBeginInfo.clearValueCount = static_cast< uint32_t >(vkClearValues.size());
        vkRenderPassBeginInfo.pClearValues = vkClearValues.data();

        // Start render pass.
        vkCmdBeginRenderPass(vkCommandBuffers[i], &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        // Bind a pipeline we defined above.
        vkCmdBindPipeline(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);
        // Bind vertices.
        VkBuffer vertexBuffers[] = { vkVertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vkCommandBuffers[i], 0, 1, vertexBuffers, offsets);
        // Bind descriptor sets for uniforms.
        vkCmdBindDescriptorSets(vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout, 0, 1, &vkDescriptorSets[i], 0, nullptr);
        // Draw command.
        vkCmdDraw(vkCommandBuffers[i], static_cast< uint32_t >(vertices.size()), 1, 0, 0);
        // Finish render pass.
        vkCmdEndRenderPass(vkCommandBuffers[i]);

        // Fihish adding commands into the buffer.
        if (vkEndCommandBuffer(vkCommandBuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to finish command buffer recording" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    //                   STEP 34: Synchronization primitives
    // ==========================================================================
    // Rendering and presentation are not synchronized. It means that if the
    // application renders frames faster then they are displayed, it will lead
    // to memory overflow. In order to avoid this, we should wait in case
    // rendering goes too fast and the chain is overflown.
    // ==========================================================================

    // Describe a semaphore.
    VkSemaphoreCreateInfo vkSemaphoreInfo{};
    vkSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // The first semaphore group signals that an image is aquired and ready for rendering.
    // Create semaphore per each image we expect to render in parallel.
    // These semaphores perform GPU-GPU synchronization.
    std::vector< VkSemaphore > vkImageAvailableSemaphores;
    vkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Create a semaphore.
        if (vkCreateSemaphore(vkDevice, &vkSemaphoreInfo, nullptr, &vkImageAvailableSemaphores[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a semaphore!" << std::endl;
            abort();
        }
    }

    // The second semaphore group signals that an image is rendered and ready for presentation.
    // Create semaphore per each image we expect to render in parallel.
    // These semaphores perform GPU-GPU synchronization.
    std::vector< VkSemaphore > vkRenderFinishedSemaphores;
    vkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Create a semaphore.
        if (vkCreateSemaphore(vkDevice, &vkSemaphoreInfo, nullptr, &vkRenderFinishedSemaphores[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a semaphore!" << std::endl;
            abort();
        }
    }

    // In order to not overflow the swap chain we need to wait on CPU side if there are too many images
    // produced by GPU. This CPU-GPU synchronization is performed by fences.

    // Free fences for images running in parallel.
    std::vector< VkFence > vkInFlightFences;
    // Buffer of feces, locked by the images running in parallel.
    std::vector< VkFence > vkImagesInFlight;

    // Describe a fence.
    VkFenceCreateInfo vkFenceInfo{};
    vkFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Create fences.
    vkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    vkImagesInFlight.resize(vkSwapChainImages.size(), VK_NULL_HANDLE);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(vkDevice, &vkFenceInfo, nullptr, &vkInFlightFences[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create a fence!" << std::endl;
            abort();
        }
    }

    // ==========================================================================
    //                     STEP 35: Pick graphics queues
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
    //                         STEP 36: Main loop
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
        vkWaitForFences(vkDevice, 1, &vkInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        // Aquire a next image from a swap chain to process.
        uint32_t imageIndex;
        vkAcquireNextImageKHR(vkDevice, vkSwapChain, UINT64_MAX, vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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
        vkMapMemory(vkDevice, vkUniformBuffersMemory[imageIndex], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(vkDevice, vkUniformBuffersMemory[imageIndex]);

        // If the image is locked - wait for it.
        if (vkImagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(vkDevice, 1, &vkImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        // Put a free fence to imagesInFlight array.
        vkImagesInFlight[imageIndex] = vkInFlightFences[currentFrame];

        // Describe a submit to the graphics queue.
        VkSubmitInfo vkSubmitInfo{};
        vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        // Specify semaphores the GPU should wait before executing the submit.
        std::array< VkSemaphore, 1 > vkWaitSemaphores{ vkImageAvailableSemaphores[currentFrame] };
        // Pipeline stages corresponding to each semaphore.
        std::array< VkPipelineStageFlags, 1 > vkWaitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        vkSubmitInfo.waitSemaphoreCount = vkWaitSemaphores.size();
        vkSubmitInfo.pWaitSemaphores = vkWaitSemaphores.data();
        vkSubmitInfo.pWaitDstStageMask = vkWaitStages.data();
        vkSubmitInfo.commandBufferCount = 1;
        vkSubmitInfo.pCommandBuffers = &vkCommandBuffers[imageIndex];
        // Specify semaphores the GPU should unlock after executing the submit.
        std::array< VkSemaphore, 1 > vkSignalSemaphores{ vkRenderFinishedSemaphores[currentFrame] };
        vkSubmitInfo.signalSemaphoreCount = vkSignalSemaphores.size();
        vkSubmitInfo.pSignalSemaphores = vkSignalSemaphores.data();

        // Reset the fence.
        vkResetFences(vkDevice, 1, &vkInFlightFences[currentFrame]);

        // Submit to the queue.
        if (vkQueueSubmit(vkGraphicsQueue, 1, &vkSubmitInfo, vkInFlightFences[currentFrame]) != VK_SUCCESS) {
            std::cerr << "Failed to submit" << std::endl;
            abort();
        }

        // Prepare an image for presentation.
        VkPresentInfoKHR vkPresentInfo{};
        vkPresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        // Specify semaphores we need to wait before presenting the image.
        vkPresentInfo.waitSemaphoreCount = vkSignalSemaphores.size();
        vkPresentInfo.pWaitSemaphores = vkSignalSemaphores.data();
        std::array< VkSwapchainKHR, 1 > swapChains{ vkSwapChain };
        vkPresentInfo.swapchainCount = swapChains.size();
        vkPresentInfo.pSwapchains = swapChains.data();
        vkPresentInfo.pImageIndices = &imageIndex;
        vkPresentInfo.pResults = nullptr;

        // Submit and image for presentaion.
        vkQueuePresentKHR(vkPresentQueue, &vkPresentInfo);

        // Switch to the next frame in the loop.
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // ==========================================================================
    //                     STEP 37: Deinitialization
    // ==========================================================================
    // Destroy all created Vukan structures in a reverse order.
    // ==========================================================================

    // Wait until all pending render operations are finished.
    vkDeviceWaitIdle(vkDevice);

    // Destroy fences.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(vkDevice, vkInFlightFences[i], nullptr);
    }

    // Destroy semaphores.
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkDevice, vkRenderFinishedSemaphores[i], nullptr);
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkDevice, vkImageAvailableSemaphores[i], nullptr);
    }

    // Destroy swap uniform buffers.
    for (size_t i = 0; i < vkSwapChainImages.size(); i++) {
        vkDestroyBuffer(vkDevice, vkUniformBuffers[i], nullptr);
        vkFreeMemory(vkDevice, vkUniformBuffersMemory[i], nullptr);
    }

    // Destory descriptor pool for uniforms.
    vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);

    // Destroy vertex buffer.
    vkDestroyBuffer(vkDevice, vkVertexBuffer, nullptr);
    vkFreeMemory(vkDevice, vkVertexBufferMemory, nullptr);

    // Destory command pool
    vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);

    // Destory framebuffers.
    for (auto framebuffer : vkSwapChainFramebuffers) {
        vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
    }

    vkDestroyImageView(vkDevice, colorImageView, nullptr);
    vkDestroyImage(vkDevice, colorImage, nullptr);
    vkFreeMemory(vkDevice, colorImageMemory, nullptr);

    // Destroy depth-stensil image and image view.
    vkDestroyImageView(vkDevice, vkDepthImageView, nullptr);
    vkDestroyImage(vkDevice, vkDepthImage, nullptr);
    vkFreeMemory(vkDevice, vkDepthImageMemory, nullptr);

    // Destory pipeline.
    vkDestroyPipeline(vkDevice, vkGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
    vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);

    // Destroy shader modules.
    vkDestroyShaderModule(vkDevice, vkFragmentShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vkVertexShaderModule, nullptr);

    // Destory swap chain image views.
    for (auto imageView : vkSwapChainImageViews) {
        vkDestroyImageView(vkDevice, imageView, nullptr);
    }

    // Destroy swap chain.
    vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);

    // Destory descriptor set layout for uniforms.
    vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);

    // Destory logical device.
    vkDestroyDevice(vkDevice, nullptr);

#ifdef DEBUG_MODE

    // Get pointer to an extension function vkDestroyDebugUtilsMessengerEXT.
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT == nullptr) {
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
