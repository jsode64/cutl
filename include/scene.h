#pragma once

#include "context.h"
#include "pipeline.h"
#include "renderer.h"

#include <vulkan/vulkan.h>

/**
 * A rendering scene.
 */
typedef struct CuScene {
    const CuContext* _context; /**< The source context. */

    VkCommandBuffer _commandBuffer; /**< The command buffer. */
} CuScene;

/**
 * Begins rendering to the renderer.
 *
 * @param scene The scene to begin rendering with.
 * @param renderer The target renderer.
 */
void cuSceneBeginRender(CuScene* scene, const CuRenderer* renderer);

/**
 * Ends the scene's current render.
 *
 * @param scene The scene whose render to end.
 * @param renderer The current render target.
 */
void cuSceneEndRender(CuScene* scene, const CuRenderer* renderer);

/**
 * Binds the pipeline to graphics.
 *
 * @param scene The scene to bind the pipeline to.
 * @param pipeline The pipeline to bind.
 */
void cuSceneBindPipeline(CuScene* scene, const CuPipeline* pipeline);

/**
 * Draws with the bound graphics pipeline.
 *
 * @param scene The scene to draw with.
 * @param nVertices The number of vertices to draw.
 * @param nInstances The number of instances.
 */
void cuSceneDraw(CuScene* scene, uint32_t nVertices, uint32_t nInstances);

/**
 * Sets the push constants.
 *
 * @param scene The scene to set the push constants for.
 * @param pipelineLayout The pipeline layout to set the push constants for.
 * @param size The size of the push constants.
 * @param data The push constants.
 */
void cuSceneSetPushConstants(
    CuScene* scene, const CuPipelineLayout* pipelineLayout, uint32_t size, const void* data
);
