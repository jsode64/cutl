#pragma once

#include "result.h"
#include "window.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

/**
 * A Vulkan context.
 */
typedef struct CuContext {
    const CuWindow* _window; /**< The source window. */

    VkInstance _instance; /**< The Vulkan instance. */

    VkDebugUtilsMessengerEXT
        _debugMessenger; /**< The debug messenger. Is unused unless `useValidation` is true. */

    VkSurfaceKHR _surface; /**< The window surface. */

    VkPhysicalDevice _physicalDevice; /**< The chosen physical device. */

    uint32_t _mainQueueIndex; /**< The main (graphics + compute + transfer) queue family index. */

    uint32_t _presentQueueIndex; /**< The present queue family index. */

    VkPhysicalDeviceMemoryProperties _memoryProperties; /**< The device's memory properties. */

    VkDevice _device; /**< The logical device. */

    VkQueue _mainQueue; /**< The main (graphics compute and transfer) queue. */

    VkQueue _presentQueue; /**< The present queue. */

    VkCommandPool _commandPool; /**< The command pool for the main queue. */
} CuContext;

/**
 * A function for judging a physical device.
 * Returns higher values for better devices, any negative value for unsuitable devices.
 *
 * @param nExtensions The number of extensions.
 * @param extensions An array of the physical device's extension properties.
 * @param features The physical device's features.
 * @param properties The physical device's properties.
 *
 * @return The physical device's score or a negative number if it's unsuitable.
 */
typedef int64_t (*CuPhysicalDeviceJudgeFn)(
    size_t nExtensions,
    const VkExtensionProperties* extensions,
    const VkPhysicalDeviceFeatures2* features,
    const VkPhysicalDeviceProperties2* properties
);

/**
 * Returns a score for the physical device, or a negative number if it's unsuitable.
 *
 * This function returns `INT64_MIN` for devices that are missing any of:
 * swapchain support,
 * dynamic rendering,
 * or synchronization 2.
 *
 * For suitable devices, returns `INT64_MAX` for discrete GPUs, 1 for others.
 *
 * @param nExtensions The number of extensions.
 * @param extensions An array of the physical device's extension properties.
 * @param features The physical device's features.
 * @param properties The physical device's properties.
 *
 * @return The physical device's score or a negative number if it's unsuitable.
 */
int64_t cuDefaultJudgePhysicalDevice(
    size_t nExtensions,
    const VkExtensionProperties* extensions,
    const VkPhysicalDeviceFeatures2* features,
    const VkPhysicalDeviceProperties2* properties
);

/**
 * Information for creating a Cuttle context.
 */
typedef struct CuContextCreateInfo {
    const CuWindow*
        window; /**< The window target. If null, presentation will not be needed/initialized. */

    const char* appName; /**< The name of the application. Can be null. */

    uint32_t appVersion; /**< The version of the application made with `VK_MAKE_API_VERSION`. Can be
                            zero. */

    CuPhysicalDeviceJudgeFn physicalDeviceJudgeFn; /**< The function for judging the physical
                                                      devices. Will use default if null. */

    bool useValidation; /**< Whether to enable validation layers and debug messaging. */
} CuContextCreateInfo;

/**
 * Creates the Vulkan context.
 *
 * @param context The context to create.
 * @param info The creation info for the context. Must not be null.
 *
 * @return The result of the context creation.
 */
CuResult cuCreateContext(CuContext* context, const CuContextCreateInfo* info);

/**
 * Destroys and uninitializes the context.
 *
 * @param context The context to destroy.
 */
void cuDestroyContext(CuContext* context);

/**
 * Finds the index of the first memory type that matches the properties and filter.
 *
 * @param memoryProperties The target device's memory properties.
 * @param filter The bitmask to filer memory types.
 * @param properties The bitmask of memory properties.
 * @param memoryTypeIndex The index to be set.
 *
 * @return Whether a matching memory type was found.
 */
bool cuFindMemoryType(
    const VkPhysicalDeviceMemoryProperties* memoryProperties,
    uint32_t filter,
    VkMemoryPropertyFlags properties,
    uint32_t* memoryTypeIndex
);

/**
 * Waits for the context to idle.
 *
 * @param context The context to wait on.
 */
static inline void cuContextWaitIdle(const CuContext* context) {
    vkDeviceWaitIdle(context->_device);
}
