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
#define CU_NULL_DYNAMIC_VBO                                                                        \
    ((CuDynamicVbo){                                                                               \
        ._renderer = NULL,                                                                         \
        ._allocation = CU_NULL_ALLOCATION,                                                         \
        ._buffer = VK_NULL_HANDLE,                                                                 \
        ._size = 0,                                                                                \
        ._address = 0,                                                                             \
    })

CuResult cuCreateDynamicVbo(
    const CuRenderer* renderer,
    uint64_t size,
    void* allocator,
    const CuAllocationCallbacks* allocationCallbacks,
    CuDynamicVbo* vbo
) {
    const VkDevice device = renderer->_context->_device;
    CuResult result = CU_SUCCESS;

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
        &vbo->_allocation
    );
    if (result.tag != CU_TAG_SUCCESS) {
        goto FAIL;
    }

    // Create the buffer.
    const VkResult createBufferResult = vkCreateBuffer(device, &createInfo, NULL, &vbo->_buffer);
    if (createBufferResult != VK_SUCCESS) {
        result = CU_VK_RESULT(createBufferResult);
        goto FAIL;
    }

    // Bind the buffer to the allocation.
    const VkResult bindBufferMemoryResult = vkBindBufferMemory(
        device, vbo->_buffer, vbo->_allocation._memory, vbo->_allocation._offset
    );
    if (bindBufferMemoryResult != VK_SUCCESS) {
        result = CU_VK_RESULT(bindBufferMemoryResult);
        goto FAIL;
    }

    // Get the buffer's device address.
    const VkBufferDeviceAddressInfo addressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = vbo->_buffer,
    };
    vbo->_address = vkGetBufferDeviceAddress(device, &addressInfo);

    vbo->_renderer = renderer;
    vbo->_size = size;

    return CU_SUCCESS;

FAIL:
    cuDestroyDynamicVbo(vbo, allocator, allocationCallbacks);
    return result;
}

void cuDestroyDynamicVbo(
    CuDynamicVbo* vbo, void* allocator, const CuAllocationCallbacks* allocationCallbacks
) {
    allocationCallbacks->freeCallback(allocator, &vbo->_allocation);
    vkDestroyBuffer(vbo->_renderer->_context->_device, vbo->_buffer, NULL);

    *vbo = CU_NULL_DYNAMIC_VBO;
}

CuResult cuDynamicVboWrite(CuDynamicVbo* vbo, const uint64_t size, const void* src) {
    const VkDevice device = vbo->_renderer->_context->_device;

    // Get the mapped memory.
    uint8_t* dst;
    const uint64_t offset = cuRendererFrameIndex(vbo->_renderer) * vbo->_size;
    const VkResult mapMemoryResult = vkMapMemory(
        device,
        vbo->_allocation._memory,
        vbo->_allocation._offset + offset,
        (VkDeviceSize)size,
        0,
        (void**)&dst
    );
    if (mapMemoryResult != VK_SUCCESS) {
        return CU_VK_RESULT(mapMemoryResult);
    }

    // Write the data.
    memcpy(dst, src, size);

    // Unmap the memory.
    vkUnmapMemory(device, vbo->_allocation._memory);

    return CU_SUCCESS;
}

CuResult cuDynamicVboWriteAll(CuDynamicVbo* vbo, uint64_t size, const void* src) {
    const VkDevice device = vbo->_renderer->_context->_device;
    const uint64_t nFrames = (uint64_t)vbo->_renderer->_nFrames;

    // Get the mapped memory.
    uint8_t* dst;
    const uint64_t offset = cuRendererFrameIndex(vbo->_renderer) * vbo->_size;
    const uint64_t bufferSize = nFrames * vbo->_size;
    const VkResult mapMemoryResult = vkMapMemory(
        device,
        vbo->_allocation._memory,
        vbo->_allocation._offset + offset,
        (VkDeviceSize)bufferSize,
        0,
        (void**)&dst
    );
    if (mapMemoryResult != VK_SUCCESS) {
        return CU_VK_RESULT(mapMemoryResult);
    }

    // Write the data.
    for (uint64_t i = 0; i < nFrames; i++) {
        memcpy(dst + (i * vbo->_size), src, size);
    }

    // Unmap the memory.
    vkUnmapMemory(device, vbo->_allocation._memory);

    return CU_SUCCESS;
}
