#include "context.h"

#include "info.h"
#include "memory.h"
#include "util.h"
#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

/**
 * An uninitialized context.
 */
#define CU_NULL_CONTEXT                                                                            \
    ((CuContext){                                                                                  \
        ._window = NULL,                                                                           \
        ._instance = VK_NULL_HANDLE,                                                               \
        ._debugMessenger = VK_NULL_HANDLE,                                                         \
        ._surface = VK_NULL_HANDLE,                                                                \
        ._physicalDevice = VK_NULL_HANDLE,                                                         \
        ._mainQueueIndex = 0,                                                                      \
        ._presentQueueIndex = 0,                                                                   \
        ._memoryProperties = {},                                                                   \
        ._device = VK_NULL_HANDLE,                                                                 \
        ._mainQueue = VK_NULL_HANDLE,                                                              \
        ._presentQueue = VK_NULL_HANDLE,                                                           \
        ._commandPool = VK_NULL_HANDLE,                                                            \
    })

/**
 * Creates   instance.
 *
 * @param info The context create info.
 * @param instance The instance to create.
 *
 * @return The result of any failed Vulkan operation, or `VK_SUCCESS` on success.
 */
static VkResult cuCreateInstance(CuContext* context, const CuContextCreateInfo* info);

/**
 * Returns the required extensions in a chunk.
 *
 * @param info The context create info.
 *
 * @return A chunk of extension names.
 */
static CuChunk cuGetInstanceExtensions(const CuContextCreateInfo* info);

/**
 * Returns the required layers in a chunk.
 *
 * @param info The context create info.
 *
 * @return A chunk of layer names.
 */
static CuChunk cuGetInstanceLayers(const CuContextCreateInfo* info);

/**
 * Vulkan debug messenger callback.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL cuVkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData
);

/**
 * Creates the context's window surface.
 *
 * @param context The context to create the instance for.
 * @param window The window to make the surface from.
 *
 * @return The result of `glfwCreateSurfaceKHR`.
 *
 * @note If the passed window is null, `VK_SUCCESS` will be returned without creating the surface.
 */
static VkResult cuCreateSurface(CuContext* context, const CuWindow* window);

/**
 * Chooses the context's physical device.
 * This also sets the context's queue indices on success.
 *
 * @param context The context to choose a physical device and get the queue family indices for.
 * @param info The context create info.
 *
 * @return The result of any failed Vulkan operation, or `VK_SUCCESS` on success.
 */
static VkResult cuChoosePhysicalDevice(CuContext* context, const CuContextCreateInfo* info);

/**
 * Queries the physical device for its queue family indices.
 *
 * @param context The context the queue family indices are being queued for.
 * @param physicalDevice The physical device to query the queue family indices of.
 * @param mainQueueIndex The index to be set to the first graphics + compute + transfer family's.
 * @param presentQueueIndex The index to be set to the first present family's.
 * @param wereBothFound The booloean to be set to whether both queue indices were defined.
 *
 * @return The result of any failed Vulkan operation, or `VK_SUCCESS` on success.
 */
static VkResult cuQueryPhysicalDeviceQueueFamilies(
    const CuContext* context,
    VkPhysicalDevice physicalDevice,
    uint32_t* mainQueueIndex,
    uint32_t* presentQueueIndex,
    bool* wereBothFound
);

/**
 * Creates the context's device.
 *
 * @param context The context whose device to create.
 *
 * @return The result of `vkCreateDevice`.
 */
static VkResult cuCreateDevice(CuContext* context);

/**
 * Gets the context's device queues (main and present).
 *
 * @param context The context whose device queues to get.
 */
static void cuGetDeviceQueues(CuContext* context);

/**
 * Creates the context's command pool for its main queue.
 *
 * @param context The context whose command pool to create.
 *
 * @return The result of `vkCreateCommandPool`.
 */
static VkResult cuCreateCommandPool(CuContext* context);

int64_t cuDefaultJudgePhysicalDevice(
    size_t nExtensions,
    const VkExtensionProperties* extensions,
    const VkPhysicalDeviceFeatures2* features,
    const VkPhysicalDeviceProperties2* properties
) {
    // Query swapchain support.
    bool isSwapchainExtensionPresent = false;
    for (size_t i = 0; i < nExtensions; i++) {
        if (strcmp(extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            isSwapchainExtensionPresent = true;
            break;
        }
    }
    if (!isSwapchainExtensionPresent) {
        return INT64_MIN;
    }

    // Query feature support.
    const VkPhysicalDeviceVulkan11Features* features11 =
        (VkPhysicalDeviceVulkan11Features*)features->pNext;
    const VkPhysicalDeviceVulkan12Features* features12 =
        (VkPhysicalDeviceVulkan12Features*)features11->pNext;
    const VkPhysicalDeviceVulkan13Features* features13 =
        (VkPhysicalDeviceVulkan13Features*)features12->pNext;
    const VkPhysicalDeviceVulkan14Features* features14 =
        (VkPhysicalDeviceVulkan14Features*)features13->pNext;
    const bool areAllFeaturesSupported =
        features->features.fillModeNonSolid && features->features.shaderInt64 == VK_TRUE && features12->descriptorIndexing == VK_TRUE &&
        features12->scalarBlockLayout == VK_TRUE&&features12->timelineSemaphore == VK_TRUE&&
        features12->bufferDeviceAddress==VK_TRUE&&
        features13->synchronization2 == VK_TRUE && features13->dynamicRendering == VK_TRUE &&
        features13->maintenance4 == VK_TRUE && features14->maintenance5 &&
        features14->pushDescriptor == VK_TRUE;
    if (!areAllFeaturesSupported) {
        return INT64_MIN;
    }

    // Give the physical device a score.
    int64_t score = 1;
    if (properties->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score = INT64_MAX;
    }

    return score;
}

CuResult cuCreateContext(CuContext* context, const CuContextCreateInfo* info) {
    VkResult result = VK_SUCCESS;

    *context = CU_NULL_CONTEXT;
    context->_window = info->window;

    result = cuCreateInstance(context, info);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    result = cuCreateSurface(context, info->window);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    result = cuChoosePhysicalDevice(context, info);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    vkGetPhysicalDeviceMemoryProperties(context->_physicalDevice, &context->_memoryProperties);

    result = cuCreateDevice(context);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    cuGetDeviceQueues(context);

    result = cuCreateCommandPool(context);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    return CU_SUCCESS;

FAIL:
    cuDestroyContext(context);
    return (CuResult){
        .tag = CU_TAG_VK_ERROR,
        .val = result,
    };
}

void cuDestroyContext(CuContext* context) {
    if (context->_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(context->_device);

        vkDestroyCommandPool(context->_device, context->_commandPool, NULL);
        vkDestroyDevice(context->_device, NULL);
    }

    if (context->_instance != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(context->_instance, context->_surface, NULL);
    }

    if (context->_debugMessenger != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT debugMessengerDestroyFn =
            (PFN_vkDestroyDebugUtilsMessengerEXT
            )vkGetInstanceProcAddr(context->_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (debugMessengerDestroyFn != NULL) {
            debugMessengerDestroyFn(context->_instance, context->_debugMessenger, NULL);
        }
    }

    vkDestroyInstance(context->_instance, NULL);

    *context = CU_NULL_CONTEXT;
}

bool cuFindMemoryType(
    const VkPhysicalDeviceMemoryProperties* memoryProperties,
    uint32_t filter,
    VkMemoryPropertyFlags properties,
    uint32_t* memoryTypeIndex
) {
    for (uint32_t i = 0; i < memoryProperties->memoryTypeCount; i++) {
        const VkMemoryType* memoryType = &memoryProperties->memoryTypes[i];
        const bool matchesFilter = (filter & (1 << i)) != 0;
        const bool matchesProperties = (memoryType->propertyFlags & properties) == properties;
        if (matchesFilter && matchesProperties) {
            *memoryTypeIndex = i;
            return true;
        }
    }

    return false;
}

VkResult cuCreateInstance(CuContext* context, const CuContextCreateInfo* info) {
    CuChunk CU_CHUNK_AUTO_FREE extensions = cuGetInstanceExtensions(info);
    CuChunk CU_CHUNK_AUTO_FREE layers = cuGetInstanceLayers(info);
    const uint32_t cuVersion =
        VK_MAKE_API_VERSION(0, CU_VERSION_MAJOR, CU_VERSION_MINOR, CU_VERSION_PATCH);
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    const VkDebugUtilsMessageTypeFlagBitsEXT messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    const VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = NULL,
        .flags = 0,
        .messageSeverity = messageSeverity,
        .messageType = messageType,
        .pfnUserCallback = cuVkDebugCallback,
        .pUserData = NULL,
    };
    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = info->appName,
        .applicationVersion = info->appVersion,
        .pEngineName = "Cuttle",
        .engineVersion = cuVersion,
        .apiVersion = VK_API_VERSION_1_4,
    };
    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = info->useValidation ? &debugMessengerCreateInfo : NULL,
        .flags = onApple ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = (uint32_t)layers._n,
        .ppEnabledLayerNames = (const char**)layers._data,
        .enabledExtensionCount = (uint32_t)extensions._n,
        .ppEnabledExtensionNames = (const char**)extensions._data,
    };

    // Create the instance and debug messenger if debugging.
    VkResult result = VK_SUCCESS;
    result = vkCreateInstance(&createInfo, NULL, &context->_instance);
    if (result != VK_SUCCESS) {
        return result;
    }
    if (info->useValidation) {
        PFN_vkCreateDebugUtilsMessengerEXT debugMessengerCreateFn =
            (PFN_vkCreateDebugUtilsMessengerEXT
            )vkGetInstanceProcAddr(context->_instance, "vkCreateDebugUtilsMessengerEXT");
        if (debugMessengerCreateFn == NULL) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        } else {
            result = debugMessengerCreateFn(
                context->_instance, &debugMessengerCreateInfo, NULL, &context->_debugMessenger
            );
        }
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    return VK_SUCCESS;
}

CuChunk cuGetInstanceExtensions(const CuContextCreateInfo* info) {
    // Get the number of extensions.
    size_t nExtensions = 0;
    if (info->useValidation) {
        nExtensions += 1;
    }
    uint32_t nGlfwExtensions = 0;
    if (onApple) {
        nExtensions += 3;
    } else {
        uint32_t nGlfwExtensions = 0;
        glfwGetRequiredInstanceExtensions(&nGlfwExtensions);
        nExtensions += (size_t)nGlfwExtensions;
    }

    if (nExtensions == 0) {
        return CU_CHUNK_NULL;
    }

    // Store extensions.
    const char** extensions = malloc(nExtensions * sizeof(const char*));
    if (extensions == NULL) {
        return CU_CHUNK_NULL;
    }
    size_t i = 0;
    if (info->useValidation) {
        extensions[i++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
    if (onApple) {
        extensions[i++] = VK_KHR_SURFACE_EXTENSION_NAME;
        extensions[i++] = "VK_EXT_metal_surface";
        extensions[i++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    } else {
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&nGlfwExtensions);
        memcpy(extensions + i, glfwExtensions, nGlfwExtensions * sizeof(const char*));
        i += (size_t)nGlfwExtensions;
    }

    return (CuChunk){
        ._n = i,
        ._data = extensions,
    };
}

static CuChunk cuGetInstanceLayers(const CuContextCreateInfo* info) {
    // Get the number of layers.
    size_t nLayers = 0;
    if (info->useValidation) {
        nLayers += 1;
    }

    if (nLayers == 0) {
        return CU_CHUNK_NULL;
    }

    // Store layers.
    const char** layers = malloc(nLayers * sizeof(const char*));
    if (layers == NULL) {
        return CU_CHUNK_NULL;
    }
    size_t i = 0;
    if (info->useValidation) {
        layers[i++] = "VK_LAYER_KHRONOS_validation";
    }

    return (CuChunk){
        ._n = i,
        ._data = layers,
    };
}

VKAPI_ATTR VkBool32 VKAPI_CALL
cuVkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void*) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        fprintf(stderr, "\n%s\n", callbackData->pMessage);
    }

    return VK_FALSE;
}

VkResult cuCreateSurface(CuContext* context, const CuWindow* window) {
    if (window == NULL) {
        return VK_SUCCESS;
    }

    VkResult result =
        glfwCreateWindowSurface(context->_instance, window->_window, NULL, &context->_surface);
    return result;
}

VkResult cuChoosePhysicalDevice(CuContext* context, const CuContextCreateInfo* info) {
    VkResult result;

    // Get the physical devices.
    uint32_t nPhysicalDevices = 0;
    result = vkEnumeratePhysicalDevices(context->_instance, &nPhysicalDevices, NULL);
    if (result != VK_SUCCESS) {
        return result;
    }
    VkPhysicalDevice physicalDevices[nPhysicalDevices];
    result = vkEnumeratePhysicalDevices(context->_instance, &nPhysicalDevices, physicalDevices);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Enumerate and choose the best physical device.
    CuPhysicalDeviceJudgeFn judgeFn = info->physicalDeviceJudgeFn != NULL
                                          ? info->physicalDeviceJudgeFn
                                          : cuDefaultJudgePhysicalDevice;
    int64_t bestScore = INT64_MIN;
    VkPhysicalDevice bestPhysicalDevice = VK_NULL_HANDLE;
    uint32_t bestMainQueueIndex = 0;
    uint32_t bestPresentQueueIndex = 0;
    for (uint32_t i = 0; i < nPhysicalDevices; i++) {
        VkPhysicalDevice physicalDevice = physicalDevices[i];

        // Get the device's queue family indices.
        uint32_t mainQueueIndex = 0, presentQueueIndex = 0;
        bool wereBothFound = false;
        result = cuQueryPhysicalDeviceQueueFamilies(
            context, physicalDevice, &mainQueueIndex, &presentQueueIndex, &wereBothFound
        );
        if (result != VK_SUCCESS) {
            return result;
        }

        // Get the physical device's score.
        uint32_t nExtensions = 0;
        result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &nExtensions, NULL);
        if (result != VK_SUCCESS) {
            return result;
        }
        VkExtensionProperties* extensions CU_AUTO_FREE =
            malloc(nExtensions * sizeof(VkExtensionProperties));
        if (extensions == NULL) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        result =
            vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &nExtensions, extensions);
        if (result != VK_SUCCESS) {
            return result;
        }
        VkPhysicalDeviceVulkan14Features features14 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
            .pNext = NULL,
        };
        VkPhysicalDeviceVulkan13Features features13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &features14,
        };
        VkPhysicalDeviceVulkan12Features features12 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = &features13,
        };
        VkPhysicalDeviceVulkan11Features features11 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = &features12,
        };
        VkPhysicalDeviceFeatures2 features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &features11,
        };
        vkGetPhysicalDeviceFeatures2(physicalDevice, &features);
        VkPhysicalDeviceVulkan14Properties properties14 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES,
            .pNext = NULL,
        };
        VkPhysicalDeviceVulkan13Properties properties13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES,
            .pNext = &properties14,
        };
        VkPhysicalDeviceVulkan12Properties properties12 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
            .pNext = &properties13,
        };
        VkPhysicalDeviceVulkan11Properties properties11 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
            .pNext = &properties12,
        };
        VkPhysicalDeviceProperties2 properties = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &properties11,
        };
        vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
        int64_t score = judgeFn(nExtensions, extensions, &features, &properties);

        // Choose the physical device if it's suitable and better scoring.
        if (score >= 0 && score >= bestScore) {
            bestScore = score;
            bestPhysicalDevice = physicalDevice;
            bestMainQueueIndex = mainQueueIndex;
            bestPresentQueueIndex = presentQueueIndex;
        }
    }

    // Make sure at least one suitable device was found.
    if (bestScore < 0) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    context->_physicalDevice = bestPhysicalDevice;
    context->_mainQueueIndex = bestMainQueueIndex;
    context->_presentQueueIndex = bestPresentQueueIndex;

    return VK_SUCCESS;
}

VkResult cuQueryPhysicalDeviceQueueFamilies(
    const CuContext* context,
    const VkPhysicalDevice physicalDevice,
    uint32_t* mainQueueIndex,
    uint32_t* presentQueueIndex,
    bool* wereBothFound
) {
    // Get the queue family indices.
    uint32_t nQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, NULL);
    VkQueueFamilyProperties* queueFamilies CU_AUTO_FREE =
        malloc(nQueueFamilies * sizeof(VkQueueFamilyProperties));
    if (queueFamilies == NULL) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilies, queueFamilies);

    // Look for matching queue families.
    bool isMainFound = false;
    bool isPresentFound = false;
    for (uint32_t i = 0; i < nQueueFamilies; i++) {
        // Query main queue support.
        const VkQueueFlagBits mainQueueFlags =
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        if (!isMainFound && (queueFamilies[i].queueFlags & mainQueueFlags) == mainQueueFlags) {
            *mainQueueIndex = i;
            isMainFound = true;
        }

        // Query present support.
        if (!isPresentFound) {
            const int hasPresentSupport =
                glfwGetPhysicalDevicePresentationSupport(context->_instance, physicalDevice, i);
            if (hasPresentSupport == GLFW_TRUE) {
                *presentQueueIndex = i;
                isPresentFound = true;
            }
        }
    }

    *wereBothFound = isMainFound && isPresentFound;

    return VK_SUCCESS;
}

VkResult cuCreateDevice(CuContext* context) {
    const float queuePriority = 1.0f;
    const uint32_t nQueues = (context->_mainQueueIndex == context->_presentQueueIndex ||
                              context->_surface != VK_NULL_HANDLE)
                                 ? 1
                                 : 2;
    const VkDeviceQueueCreateInfo queueCreateInfos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = context->_mainQueueIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        },
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = context->_presentQueueIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        }
    };
    const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};
    const uint32_t nExtensions = onApple ? 2 : 1;
    const VkPhysicalDeviceVulkan14Features features14 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = NULL,
        .maintenance5 = VK_TRUE,
        .pushDescriptor = VK_TRUE,
    };
    const VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = (void*)&features14,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
        .maintenance4 = VK_TRUE,
    };
    const VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = (void*)&features13,
        .descriptorIndexing = VK_TRUE,
        .scalarBlockLayout = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };
    const VkPhysicalDeviceVulkan11Features features11 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = (void*)&features12,
    };
    const VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = (void*)&features11,
        .features =
            {
                .fillModeNonSolid = VK_TRUE,
                .shaderInt64 = VK_TRUE,
            },
    };
    const VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .flags = 0,
        .queueCreateInfoCount = nQueues,
        .pQueueCreateInfos = queueCreateInfos,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = nExtensions,
        .ppEnabledExtensionNames = extensions,
    };

    return vkCreateDevice(context->_physicalDevice, &createInfo, NULL, &context->_device);
}

void cuGetDeviceQueues(CuContext* context) {
    vkGetDeviceQueue(context->_device, context->_mainQueueIndex, 0, &context->_mainQueue);

    if (context->_window != NULL) {
        vkGetDeviceQueue(context->_device, context->_presentQueueIndex, 0, &context->_presentQueue);
    } else {
        context->_presentQueue = VK_NULL_HANDLE;
    }
}

VkResult cuCreateCommandPool(CuContext* context) {
    const VkCommandPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = context->_mainQueueIndex,
    };

    return vkCreateCommandPool(context->_device, &createInfo, NULL, &context->_commandPool);
}
