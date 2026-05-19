#include "renderer.h"

#include "context.h"
#include "scene.h"
#include "swapchain.h"
#include "util.h"

#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

/**
 * An uninitialized renderer.
 */
#define CU_NULL_RENDERER                                                                           \
    ((CuRenderer){                                                                                 \
        ._context = NULL,                                                                          \
        ._swapchain = VK_NULL_HANDLE,                                                              \
        ._swapchainExtent = {},                                                                    \
        ._swapchainFormat = 0,                                                                     \
        ._nSwapchainImages = 0,                                                                    \
        ._swapchainImages = NULL,                                                                  \
        ._nFrames = 0,                                                                             \
        ._frames = NULL,                                                                           \
        ._timelineSemaphore = VK_NULL_HANDLE,                                                      \
        ._frameCounter = 0,                                                                        \
        ._imageIndex = 0,                                                                          \
    })

/**
 * The longest a stall will wait before timing out.
 */
#define CU_TIMEOUT_NANOS 1000000000

/**
 * Retreives the swapchain images and crates their views, all stored in an array.
 *
 * @param renderer The renderer whose swapchain images to create.
 * @param format The swapchain image format.
 *
 * @return `VK_SUCCESS` on success, or the encountered error on failure.
 */
static VkResult cuCreateSwapchainImages(CuRenderer* renderer, VkFormat format);

/**
 * Creates the renderer's frame in flight objects.
 *
 * @param renderer The renderer whose frame in flight objects to create.
 * @param nFrames The number of frames in flight to create.
 *
 * @return `VK_SUCCESS` on success, or the encountered error on failure.
 */
static VkResult cuCreateFrames(CuRenderer* renderer, uint32_t nFrames);

/**
 * Creates the renderer's timeline semaphore.
 *
 * @param renderer The renderer whose timeline semaphore to create.
 *
 * @return `VK_SUCCESS` on success, or the encountered error on failure.
 */
static VkResult cuCreateTimelineSemaphore(CuRenderer* renderer);

CuResult
cuCreateRenderer(CuRenderer* renderer, const CuContext* context, const CuRendererCreateInfo* info) {
    VkResult result = VK_SUCCESS;

    *renderer = CU_NULL_RENDERER;
    renderer->_context = context;

    CuSwapchainInfo swapchainInfo = {};
    result = cuGetSwapchainInfo(&swapchainInfo, context);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    result = cuCreateSwapchain(&renderer->_swapchain, context, &swapchainInfo);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    renderer->_swapchainExtent = swapchainInfo.extent;
    renderer->_swapchainFormat = swapchainInfo.surfaceFormat.format;

    result = cuCreateSwapchainImages(renderer, swapchainInfo.surfaceFormat.format);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    result = cuCreateFrames(renderer, info->nFrames);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    result = cuCreateTimelineSemaphore(renderer);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    renderer->_frameCounter = renderer->_nFrames;
    renderer->_imageIndex = 0;

    return CU_SUCCESS;

FAIL:
    cuDestroyRenderer(renderer);
    return (CuResult){
        .tag = CU_TAG_VK_ERROR,
        .val = result,
    };
}

void cuDestroyRenderer(CuRenderer* renderer) {
    const VkDevice device = renderer->_context->_device;
    vkDeviceWaitIdle(device);

    vkDestroySemaphore(device, renderer->_timelineSemaphore, NULL);

    // Destroy frame objects.
    if (renderer->_frames != NULL) {
        for (uint32_t i = 0; i < renderer->_nFrames; i++) {
            const CuFrame* frame = &renderer->_frames[i];
            vkFreeCommandBuffers(
                device, renderer->_context->_commandPool, 1, &frame->_commandBuffer
            );
            vkDestroySemaphore(device, frame->_imageAvailable, NULL);
        }
        free(renderer->_frames);
    }

    // Destroy swapchain images.
    if (renderer->_swapchainImages != NULL) {
        for (uint32_t i = 0; i < renderer->_nSwapchainImages; i++) {
            const CuSwapchainImage* swapchainImage = &renderer->_swapchainImages[i];
            vkDestroySemaphore(device, swapchainImage->_renderFinished, NULL);
            vkDestroyImageView(device, swapchainImage->_imageView, NULL);
        }
        free(renderer->_swapchainImages);
    }

    vkDestroySwapchainKHR(device, renderer->_swapchain, NULL);

    *renderer = CU_NULL_RENDERER;
}

CuResult cuRendererBeginScene(CuRenderer* renderer, CuScene* scene) {
    const VkDevice device = renderer->_context->_device;
    const CuFrame* frame = &renderer->_frames[renderer->_frameCounter % renderer->_nFrames];
    const VkCommandBuffer commandBuffer = frame->_commandBuffer;
    VkResult result = VK_SUCCESS;

    scene->_context = renderer->_context;
    scene->_commandBuffer = commandBuffer;

    // Wait for the frame to be free.
    const uint64_t waitValue = renderer->_frameCounter + 1 - renderer->_nFrames;
    const VkSemaphoreWaitInfo waitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = NULL,
        .flags = 0,
        .semaphoreCount = 1,
        .pSemaphores = &renderer->_timelineSemaphore,
        .pValues = &waitValue,
    };
    result = vkWaitSemaphores(device, &waitInfo, CU_TIMEOUT_NANOS);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    // Get the next swapchain image index.
    result = vkAcquireNextImageKHR(
        device,
        renderer->_swapchain,
        CU_TIMEOUT_NANOS,
        frame->_imageAvailable,
        VK_NULL_HANDLE,
        &renderer->_imageIndex
    );
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    // Get and begin the commmand buffer.
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    return CU_SUCCESS;

FAIL:
    return (CuResult){
        .tag = CU_TAG_VK_ERROR,
        .val = result,
    };
}

CuResult cuRendererSubmitScene(CuRenderer* renderer, CuScene* scene) {
    const VkCommandBuffer commandBuffer = scene->_commandBuffer;
    const CuFrame* frame = &renderer->_frames[renderer->_frameCounter % renderer->_nFrames];
    const CuSwapchainImage* swapchainImage = &renderer->_swapchainImages[renderer->_imageIndex];
    VkResult result = VK_SUCCESS;

    // End the command buffer.
    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    // Submit the commands.
    const VkSemaphoreSubmitInfo waitSemaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = NULL,
        .semaphore = frame->_imageAvailable,
        .value = 0,
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .deviceIndex = 0,
    };
    const VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = NULL,
        .commandBuffer = commandBuffer,
        .deviceMask = 1,
    };
    const VkSemaphoreSubmitInfo signalSemaphoreInfos[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = NULL,
            .semaphore = swapchainImage->_renderFinished,
            .value = 0,
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .deviceIndex = 0,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = NULL,
            .semaphore = renderer->_timelineSemaphore,
            .value = renderer->_frameCounter + 1,
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .deviceIndex = 0,
        },
    };
    const VkSubmitInfo2 submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = NULL,
        .flags = 0,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 2,
        .pSignalSemaphoreInfos = signalSemaphoreInfos,
    };
    result = vkQueueSubmit2(renderer->_context->_mainQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }
    renderer->_frameCounter += 1;

    // Submit presentation.
    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchainImage->_renderFinished,
        .swapchainCount = 1,
        .pSwapchains = &renderer->_swapchain,
        .pImageIndices = &renderer->_imageIndex,
        .pResults = NULL,
    };
    result = vkQueuePresentKHR(renderer->_context->_presentQueue, &presentInfo);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    return CU_SUCCESS;

FAIL:
    return (CuResult){
        .tag = CU_TAG_VK_ERROR,
        .val = result,
    };
}

VkResult cuCreateSwapchainImages(CuRenderer* renderer, VkFormat format) {
    const VkDevice device = renderer->_context->_device;
    VkResult result = VK_SUCCESS;

    // Get the images.
    uint32_t nImages = 0;
    result =
        vkGetSwapchainImagesKHR(renderer->_context->_device, renderer->_swapchain, &nImages, NULL);
    if (result != VK_SUCCESS) {
        return result;
    }
    VkImage* images CU_AUTO_FREE = malloc(nImages * sizeof(VkImage));
    if (images == NULL) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    result = vkGetSwapchainImagesKHR(
        renderer->_context->_device, renderer->_swapchain, &nImages, images
    );
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate and initialize the swapchain image array.
    renderer->_nSwapchainImages = nImages;
    renderer->_swapchainImages = malloc(nImages * sizeof(CuSwapchainImage));
    if (renderer->_swapchainImages == NULL) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Initialize the swapchain images.
    for (uint32_t i = 0; i < nImages; i++) {
        const VkComponentMapping componentMapping = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        };
        const VkImageSubresourceRange subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        const VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = componentMapping,
            .subresourceRange = subresourceRange,
        };
        const VkSemaphoreTypeCreateInfo semaphoreType = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = NULL,
            .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
            .initialValue = 0,
        };
        const VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreType,
            .flags = 0,
        };

        CuSwapchainImage* swapchainImage = &renderer->_swapchainImages[i];
        swapchainImage->_image = images[i];
        result = vkCreateImageView(device, &imageViewCreateInfo, NULL, &swapchainImage->_imageView);
        if (result != VK_SUCCESS) {
            return result;
        }
        result =
            vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &swapchainImage->_renderFinished);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    return VK_SUCCESS;
}

VkResult cuCreateFrames(CuRenderer* renderer, uint32_t nFrames) {
    const VkDevice device = renderer->_context->_device;
    VkResult result = VK_SUCCESS;

    // Allocate the command buffers.
    const VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = renderer->_context->_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = nFrames,
    };
    VkCommandBuffer commandBuffers[nFrames];
    result = vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Allocate and initialize frame array.
    renderer->_nFrames = nFrames;
    renderer->_frames = malloc(nFrames * sizeof(CuFrame));
    if (renderer->_frames == NULL) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Initialize the frames.
    for (uint32_t i = 0; i < nFrames; i++) {
        const VkSemaphoreTypeCreateInfo semaphoreType = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .pNext = NULL,
            .semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
            .initialValue = 0,
        };
        const VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreType,
            .flags = 0,
        };

        CuFrame* frame = &renderer->_frames[i];
        result = vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &frame->_imageAvailable);
        if (result != VK_SUCCESS) {
            return result;
        }
        frame->_commandBuffer = commandBuffers[i];
    }

    return VK_SUCCESS;
}

VkResult cuCreateTimelineSemaphore(CuRenderer* renderer) {
    const VkSemaphoreTypeCreateInfo semaphoreType = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = NULL,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = renderer->_nFrames,
    };
    const VkSemaphoreCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &semaphoreType,
        .flags = 0,
    };

    return vkCreateSemaphore(
        renderer->_context->_device, &createInfo, NULL, &renderer->_timelineSemaphore
    );
}
