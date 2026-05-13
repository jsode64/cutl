#pragma once

#include "context.h"

#include <vulkan/vulkan.h>

/**
 * Swapchain format information.
 */
typedef struct CuSwapchainInfo {
    VkSurfaceCapabilitiesKHR capabilities; /**< The surface capabilities. */

    VkExtent2D extent; /**< The swapchain extent. */

    VkSurfaceFormatKHR surfaceFormat; /**< The surface format. */

    VkPresentModeKHR presentMode; /**< The present mode. */
} CuSwapchainInfo;

/**
 * Gets the swapchain information for the context.
 *
 * @param swapchainInfo The swapchain info to be initialized.
 * @param context The context to get the swapchain information from.
 *
 * @return The result of any failed Vulkan operation or `VK_SUCCESS`.
 */
VkResult cuGetSwapchainInfo(CuSwapchainInfo* swapchainInfo, const CuContext* context);

/**
 * Creates a swapchain from the global context.
 *
 * @param swapchain The swapchain to create.
 * @param context The context to create the swapchain from.
 *
 * @return The return value of `vkCreateSwapchainKHR`.
 */
VkResult cuCreateSwapchain(
    VkSwapchainKHR* swapchain, const CuContext* context, const CuSwapchainInfo* swapchainInfo
);
