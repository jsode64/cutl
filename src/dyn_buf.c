#include "allocation.h"
#include "buffer.h"
#include "context.h"
#include "renderer.h"
#include "result.h"
#include "util.h"

#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>

/**
 * An uninitialized dynamic vertex buffer.
 */
#define CU_NULL_DYN_BUF                                                                            \
    ((CuDynBuf){                                                                                   \
        ._renderer = NULL,                                                                         \
        ._allocation = CU_NULL_ALLOCATION,                                                         \
        ._buffer = VK_NULL_HANDLE,                                                                 \
        ._size = 0,                                                                                \
        ._address = 0,                                                                             \
    })

CuResult cuCreateDynBuf(
    const CuRenderer* const renderer,
    const uint64_t size,
    void* const allocator,
    const CuAllocationCallbacks* const allocationCallbacks,
    CuDynBuf* const buffer
) {
    const VkDevice device = renderer->_context->_device;
    CuResult result = CU_SUCCESS;

    *buffer = CU_NULL_DYN_BUF;

    // Get the memory requirements.
    const uint64_t bufferSize = size * renderer->_nFrames;
    const VkBufferCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };
    const VkDeviceBufferMemoryRequirements requirementInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
        .pNext = NULL,
        .pCreateInfo = &createInfo,
    };
    VkMemoryRequirements2 memoryRequirements = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = NULL,
        .memoryRequirements = {},
    };
    vkGetDeviceBufferMemoryRequirements(device, &requirementInfo, &memoryRequirements);

    // Allocate the memory.
    result = allocationCallbacks->allocateCallback(
        allocator,
        &memoryRequirements.memoryRequirements,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        &buffer->_allocation
    );
    if (!CU_IS_SUCCESS(result)) {
        goto FAIL;
    }

    // Create the buffer.
    const VkResult createBufferResult = vkCreateBuffer(device, &createInfo, NULL, &buffer->_buffer);
    if (createBufferResult != VK_SUCCESS) {
        result = CU_VK_ERROR(createBufferResult);
        goto FAIL;
    }

    // Bind the buffer to the allocation.
    const VkResult bindBufferMemoryResult = vkBindBufferMemory(
        device, buffer->_buffer, buffer->_allocation._memory, buffer->_allocation._offset
    );
    if (bindBufferMemoryResult != VK_SUCCESS) {
        result = CU_VK_ERROR(bindBufferMemoryResult);
        goto FAIL;
    }

    // Get the buffer's device address.
    const VkBufferDeviceAddressInfo addressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = buffer->_buffer,
    };
    buffer->_address = vkGetBufferDeviceAddress(device, &addressInfo);

    buffer->_renderer = renderer;
    buffer->_size = size;

    return CU_SUCCESS;

FAIL:
    cuDestroyDynBuf(buffer, allocator, allocationCallbacks);
    return result;
}

void cuDestroyDynBuf(
    CuDynBuf* const buffer,
    void* const allocator,
    const CuAllocationCallbacks* const allocationCallbacks
) {
    allocationCallbacks->freeCallback(allocator, &buffer->_allocation);
    vkDestroyBuffer(buffer->_renderer->_context->_device, buffer->_buffer, NULL);

    *buffer = CU_NULL_DYN_BUF;
}

CuResult cuDynBufWrite(CuDynBuf* const buffer, const uint64_t size, const void* const src) {
    const VkDevice device = buffer->_renderer->_context->_device;

    // Get the mapped memory.
    uint8_t* dst;
    const uint64_t offset = cuRendererFrameIndex(buffer->_renderer) * buffer->_size;
    const VkResult mapMemoryResult = vkMapMemory(
        device,
        buffer->_allocation._memory,
        buffer->_allocation._offset + offset,
        (VkDeviceSize)size,
        0,
        (void**)&dst
    );
    if (mapMemoryResult != VK_SUCCESS) {
        return CU_VK_ERROR(mapMemoryResult);
    }

    // Write the data.
    memcpy(dst, src, size);

    // Unmap the memory.
    vkUnmapMemory(device, buffer->_allocation._memory);

    return CU_SUCCESS;
}

CuResult cuDynBufWriteAll(CuDynBuf* const buffer, const uint64_t size, const void* const src) {
    const VkDevice device = buffer->_renderer->_context->_device;
    const uint64_t nFrames = (uint64_t)buffer->_renderer->_nFrames;

    // Get the mapped memory.
    uint8_t* dst;
    const uint64_t offset = cuRendererFrameIndex(buffer->_renderer) * buffer->_size;
    const uint64_t bufferSize = nFrames * buffer->_size;
    const VkResult mapMemoryResult = vkMapMemory(
        device,
        buffer->_allocation._memory,
        buffer->_allocation._offset + offset,
        (VkDeviceSize)bufferSize,
        0,
        (void**)&dst
    );
    if (mapMemoryResult != VK_SUCCESS) {
        return CU_VK_ERROR(mapMemoryResult);
    }

    // Write the data.
    for (uint64_t i = 0; i < nFrames; i++) {
        memcpy(dst + (i * buffer->_size), src, size);
    }

    // Unmap the memory.
    vkUnmapMemory(device, buffer->_allocation._memory);

    return CU_SUCCESS;
}
