#include "swapchain.h"

#include "context.h"
#include "memory.h"
#include "util.h"
#include "window.h"

#include <vulkan/vulkan.h>

/**
 * Queries the context's surface formats and sets the perferred one.
 *
 * @param context The source context.
 * @param surfaceFormat The surface format to be set.
 *
 * @return The result of any failed Vulkan operation or `VK_SUCCESS`.
 */
static VkResult cuChooseSurfaceFormat(VkSurfaceFormatKHR* surfaceFormat, const CuContext* context);

/**
 * Queries the context's present modes and sets the perferred one.
 *
 * @param context The source context.
 * @param presentMode The present mode to be set.
 *
 * @return The result of any failed Vulkan operation or `VK_SUCCESS`.
 */
static VkResult cuChoosePresentMode(VkPresentModeKHR* presentMode, const CuContext* context);

VkResult cuGetSwapchainInfo(CuSwapchainInfo* swapchainInfo, const CuContext* context) {
    // Get the surface capabilities.
    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        context->_physicalDevice, context->_surface, &capabilities
    );

    // Get the extent.
    VkExtent2D extent;
    const bool isExtentDefined = capabilities.maxImageExtent.width != UINT32_MAX;
    if (isExtentDefined) {
        extent = capabilities.currentExtent;
    } else {
        const VkExtent2D minExtent = capabilities.minImageExtent;
        const VkExtent2D maxExtent = capabilities.maxImageExtent;
        extent.width = CLAMP(context->_window->_width, minExtent.width, maxExtent.width);
        extent.height = CLAMP(context->_window->_height, minExtent.height, maxExtent.height);
    }

    // Choose a surface format and present mode.
    VkResult result = VK_SUCCESS;
    VkSurfaceFormatKHR surfaceFormat = {};
    result = cuChooseSurfaceFormat(&surfaceFormat, context);
    if (result != VK_SUCCESS) {
        return result;
    }
    VkPresentModeKHR presentMode = 0;
    result = cuChoosePresentMode(&presentMode, context);
    if (result != VK_SUCCESS) {
        return result;
    }

    *swapchainInfo = (CuSwapchainInfo){
        .capabilities = capabilities,
        .extent = extent,
        .surfaceFormat = surfaceFormat,
        .presentMode = presentMode,
    };

    return VK_SUCCESS;
}

VkResult cuCreateSwapchain(
    VkSwapchainKHR* swapchain, const CuContext* context, const CuSwapchainInfo* swapchainInfo
) {
    const uint32_t maxImages = (swapchainInfo->capabilities.maxImageCount == 0)
                                   ? UINT32_MAX
                                   : swapchainInfo->capabilities.maxImageCount;
    const uint32_t minImageCount = MIN(swapchainInfo->capabilities.minImageCount + 1, maxImages);
    const bool areQueueFamiliesExclusive =
        (context->_mainQueueIndex == context->_presentQueueIndex);
    const uint32_t queueFamilyIndices[] = {
        context->_mainQueueIndex,
        context->_presentQueueIndex,
    };
    const VkCompositeAlphaFlagBitsKHR compositeAlpha =
        (swapchainInfo->capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
            ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
            : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    const VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = context->_surface,
        .minImageCount = minImageCount,
        .imageFormat = swapchainInfo->surfaceFormat.format,
        .imageColorSpace = swapchainInfo->surfaceFormat.colorSpace,
        .imageExtent = swapchainInfo->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode =
            areQueueFamiliesExclusive ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = areQueueFamiliesExclusive ? 0 : 2,
        .pQueueFamilyIndices = areQueueFamiliesExclusive ? NULL : queueFamilyIndices,
        .preTransform = swapchainInfo->capabilities.currentTransform,
        .compositeAlpha = compositeAlpha,
        .presentMode = swapchainInfo->presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    return vkCreateSwapchainKHR(context->_device, &createInfo, NULL, swapchain);
}

VkResult cuChooseSurfaceFormat(VkSurfaceFormatKHR* surfaceFormat, const CuContext* context) {
    // Get the surface formats.
    VkResult result = VK_SUCCESS;
    uint32_t nSurfaceFormats = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        context->_physicalDevice, context->_surface, &nSurfaceFormats, NULL
    );
    if (result != VK_SUCCESS) {
        return result;
    }
    VkSurfaceFormatKHR* surfaceFormats CU_AUTO_FREE =
        malloc(nSurfaceFormats * sizeof(VkSurfaceFormatKHR));
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        context->_physicalDevice, context->_surface, &nSurfaceFormats, surfaceFormats
    );
    if (result != VK_SUCCESS) {
        return result;
    }

    // Choose the best format.
    for (uint32_t i = 0; i < nSurfaceFormats; i++) {
        const bool matchesFormat = surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB;
        const bool matchesColorSpace =
            surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        if (matchesFormat && matchesColorSpace) {
            *surfaceFormat = surfaceFormats[i];
            return VK_SUCCESS;
        }
    }
    for (uint32_t i = 0; i < nSurfaceFormats; i++) {
        const bool matchesFormat = surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB;
        const bool matchesColorSpace =
            surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        if (matchesFormat && matchesColorSpace) {
            *surfaceFormat = surfaceFormats[i];
            return VK_SUCCESS;
        }
    }
    for (uint32_t i = 0; i < nSurfaceFormats; i++) {
        if (surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            *surfaceFormat = surfaceFormats[i];
            return VK_SUCCESS;
        }
    }
    *surfaceFormat = surfaceFormats[0];

    return VK_SUCCESS;
}

VkResult cuChoosePresentMode(VkPresentModeKHR* presentMode, const CuContext* context) {
    // Get the present modes.
    VkResult result = VK_SUCCESS;
    uint32_t nPresentModes = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        context->_physicalDevice, context->_surface, &nPresentModes, NULL
    );
    if (result != VK_SUCCESS) {
        return result;
    }
    VkPresentModeKHR* presentModes CU_AUTO_FREE = malloc(nPresentModes * sizeof(VkPresentModeKHR));
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        context->_physicalDevice, context->_surface, &nPresentModes, presentModes
    );
    if (result != VK_SUCCESS) {
        return result;
    }

    // Choose mailbox if available, fall back to FIFO.
    for (uint32_t i = 0; i < nPresentModes; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            *presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            return VK_SUCCESS;
        }
    }
    *presentMode = VK_PRESENT_MODE_FIFO_KHR;

    return VK_SUCCESS;
}
