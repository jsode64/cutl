#include "scene.h"

#include "renderer.h"

#include <vulkan/vulkan.h>

void cuSceneBeginRender(CuScene* const scene, const CuRenderer* const renderer) {
    const CuSwapchainImage* swapchainImage = &renderer->_swapchainImages[renderer->_imageIndex];

    // Set viewport and scissor.
    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)renderer->_swapchainExtent.width,
        .height = (float)renderer->_swapchainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(scene->_commandBuffer, 0, 1, &viewport);
    const VkRect2D scissor = {
        .offset =
            {
                .x = 0,
                .y = 0,
            },
        .extent = renderer->_swapchainExtent,
    };
    vkCmdSetScissor(scene->_commandBuffer, 0, 1, &scissor);

    // Convert image for rendering.
    const VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    const VkImageMemoryBarrier2 imageBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = 0,
        .dstQueueFamilyIndex = 0,
        .image = swapchainImage->_image,
        .subresourceRange = subresourceRange,
    };
    const VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = NULL,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = NULL,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };
    vkCmdPipelineBarrier2(scene->_commandBuffer, &dependencyInfo);

    // Begin rendering.
    const VkRect2D renderArea = {
        .offset =
            {
                .x = 0,
                .y = 0,
            },
        .extent = renderer->_swapchainExtent,
    };
    const VkClearValue clearValue = {
        .color.float32 = {0.0f, 0.0f, 0.0f, 1.0f},
    };
    const VkRenderingAttachmentInfo colorAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,
        .imageView = swapchainImage->_imageView,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearValue,
    };
    const VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderArea = renderArea,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = NULL,
        .pStencilAttachment = NULL,
    };
    vkCmdBeginRendering(scene->_commandBuffer, &renderingInfo);
}

void cuSceneEndRender(CuScene* const scene, const CuRenderer* const renderer) {
    vkCmdEndRendering(scene->_commandBuffer);

    // Convert the swapchain image to presentable.
    const VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    const VkImageMemoryBarrier2 imageBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
        .dstAccessMask = VK_ACCESS_2_NONE,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = 0,
        .dstQueueFamilyIndex = 0,
        .image = renderer->_swapchainImages[renderer->_imageIndex]._image,
        .subresourceRange = subresourceRange,
    };
    const VkDependencyInfo dependencyInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = NULL,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = NULL,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };
    vkCmdPipelineBarrier2(scene->_commandBuffer, &dependencyInfo);
}

void cuSceneBindPipeline(CuScene* const scene, const CuPipeline* const pipeline) {
    vkCmdBindPipeline(scene->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->_pipeline);
}

void cuSceneDraw(CuScene* const scene, const uint32_t nVertices, const uint32_t nInstances) {
    vkCmdDraw(scene->_commandBuffer, nVertices, nInstances, 0, 0);
}

void cuSceneSetPushConstants(
    CuScene* const scene,
    const CuPipelineLayout* const pipelineLayout,
    const uint32_t size,
    const void* const data
) {
    vkCmdPushConstants(
        scene->_commandBuffer,
        pipelineLayout->_pipelineLayout,
        VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT,

        0,
        size,
        data
    );
}
