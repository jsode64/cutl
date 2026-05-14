#include "allocation.h"

#include "context.h"
#include "result.h"

#include <vulkan/vulkan.h>

CuResult cuStandardAllocatorAllocationCallback(
    void* self,
    const VkMemoryRequirements* requirements,
    VkMemoryPropertyFlags properties,
    VkMemoryAllocateFlags flags,
    CuAllocation* allocation
) {
    const CuContext* context = (const CuContext*)self;
    VkResult result = VK_SUCCESS;

    *allocation = CU_NULL_ALLOCATION;

    // Find the memory type index.
    uint32_t memoryTypeIndex = 0;
    const bool memoryTypeFound = cuFindMemoryType(
        &context->_memoryProperties, requirements->memoryTypeBits, properties, &memoryTypeIndex
    );
    if (!memoryTypeFound) {
        result = VK_ERROR_INITIALIZATION_FAILED;
        goto FAIL;
    }

    // Allocate the memory.
    const VkMemoryAllocateFlagsInfo allocateFlags = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = NULL,
        .flags = flags,
        .deviceMask = 1,
    };
    const VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &allocateFlags,
        .allocationSize = requirements->size,
        .memoryTypeIndex = memoryTypeIndex,
    };
    result = vkAllocateMemory(context->_device, &allocateInfo, NULL, &allocation->_memory);
    if (result != VK_SUCCESS) {
        goto FAIL;
    }

    allocation->_offset = 0;

    return CU_SUCCESS;

FAIL:
    *allocation = CU_NULL_ALLOCATION;
    return (CuResult){
        .tag = CU_TAG_VK_ERROR,
        .val = result,
    };
}

/**
 * An allocator memory free callback.
 *
 * @param self A void pointer to the source allocator.
 * @param allocation The allocation to free.
 */
void cuStandardAllocatorFreeCallback(void* self, CuAllocation* allocation) {
    const CuContext* context = (const CuContext*)self;

    vkFreeMemory(context->_device, allocation->_memory, NULL);

    *allocation = CU_NULL_ALLOCATION;
}
