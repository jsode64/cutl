#pragma once

#include "context.h"
#include "result.h"

/**
 * A swapchain image.
 */
typedef struct CuSwpachainImage {
    VkImage _image; /**< The image. */

    VkImageView _imageView; /**< The image view. */

    VkSemaphore _renderFinished; /**< The semaphore signalling when the render is finished. */
} CuSwapchainImage;

/**
 * A frame in flight synchronization and submition object.
 */
typedef struct CuFrame {
    VkSemaphore
        _imageAvailable; /**< The binary semaphore telling whether the image is available. */

    VkCommandBuffer _commandBuffer; /**< The frame's command buffer. */
} CuFrame;

/**
 * Forward declaration. Real is in `scene.h`.
 */
typedef struct CuScene CuScene;

/**
 * Create info for a renderer.
 */
typedef struct CuRendererCreateInfo {
    uint32_t nFrames; /**< The number of frames in flight. */
} CuRendererCreateInfo;

/**
 * A renderer.
 */
typedef struct CuRenderer {
    const CuContext* _context; /**< The source context. */

    VkSwapchainKHR _swapchain; /**< The swapchain. */

    VkExtent2D _swapchainExtent; /**< The swapchain images' extent. */

    VkFormat _swapchainFormat; /**< The swpchain images' format. */

    uint32_t _nSwapchainImages; /**< The number of swapchain images. */

    CuSwapchainImage* _swapchainImages; /**< The swapchain images. */

    uint32_t _nFrames; /**< The number of frames in flight. */

    CuFrame* _frames; /**< The frame in flight objects. */

    VkSemaphore
        _timelineSemaphore; /**< The timeline semaphore signaling when frame N is completed. */

    uint64_t _frameCounter; /**< The frame counter. */

    uint32_t _imageIndex; /**< The index of the next target swapchain image. */
} CuRenderer;

/**
 * Creates the renderer.
 *
 * @param renderer The renderer to create.
 * @param context The source context for the renderer.
 * @param info The renderer create info.
 *
 * @return The result of the renderer's creation.
 */
CuResult
cuCreateRenderer(CuRenderer* renderer, const CuContext* context, const CuRendererCreateInfo* info);

/**
 * Destroys and uninitializes the renderer.
 *
 * @param renderer The renderer to destroy.
 */
void cuDestroyRenderer(CuRenderer* renderer);

/**
 * Begins the scene for targeting the renderer's next frame.
 *
 * @param renderer The renderer.
 * @param scene The scene to initialize.
 *
 * @return `CU_SUCCESS` on success, or the encountered error on failure.
 */
CuResult cuRendererBeginScene(CuRenderer* renderer, CuScene* scene);

/**
 * Ends and submits the scene.
 *
 * @param renderer The renderer to submit the scene.
 * @param scene The scene to submit.
 *
 * @return `CU_SUCCESS` on success, or the encountered error on failure.
 */
CuResult cuRendererSubmitScene(CuRenderer* renderer, CuScene* scene);

/**
 * @param renderer The renderer to get the format of.
 *
 * @return The format of the renderer's swapchain images.
 */
static inline VkFormat cuRendererFormat(const CuRenderer* const renderer) {
    return renderer->_swapchainFormat;
}

/**
 * @param renderer The renderer to get the current frame index of.
 *
 * @return The index of the renderer's current target frame.
 */
static inline uint64_t cuRendererFrameIndex(const CuRenderer* const renderer) {
    return (renderer->_frameCounter % renderer->_nFrames);
}
