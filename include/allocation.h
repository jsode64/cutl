#pragma once

#include "result.h"

#include <vulkan/vulkan.h>

/**
 * An uninitialized allocation.
 */
#define CU_NULL_ALLOCATION                                                                         \
    ((CuAllocation){                                                                               \
        ._memory = VK_NULL_HANDLE,                                                                 \
        ._offset = 0,                                                                              \
    })

/**
 * A Vulkan GPU memory allocation.
 */
typedef struct CuAllocation {
    VkDeviceMemory _memory; /**< The memory allocation handle. */

    VkDeviceSize _offset; /**< The memory offset. */
} CuAllocation;

/**
 * A GPU allocator memory allocation callback.
 *
 * @param self A void pointer to the source allocator.
 * @param requirements The memory requirements.
 * @param properties The memory properties.
 * @param flags The memory allocation flags.
 * @param allocation The allocation to be initialized.
 *
 * @return The result of the allocation.
 */
typedef CuResult (*CuAllocatorAllocateCallback)(
    void* self,
    const VkMemoryRequirements* requirements,
    VkMemoryPropertyFlags properties,
    VkMemoryAllocateFlags flags,
    CuAllocation* allocation
);

/**
 * A GPU allocator memory free callback.
 *
 * @param self A void pointer to the source allocator.
 * @param allocation The allocation to free.
 */
typedef void (*CuAllocatorFreeCallback)(void* self, CuAllocation* allocation);

/**
 * GPU allocator callbacks.
 */
typedef struct CuAllocationCallbacks {
    CuAllocatorAllocateCallback allocateCallback; /**< The allocation callback. */

    CuAllocatorFreeCallback freeCallback; /**< The free callback. */
} CuAllocationCallbacks;

/**
 * Creates the allocation, giving it its own allocation.
 *
 * @param self The allocator (the `CuContext`).
 * @param allocation The allocation to be initialized.
 * @param requirements The memory requirements.
 * @param properties The memory properties.
 *
 * @return The result of the allocation.
 */
CuResult cuStandardAllocatorAllocationCallback(
    void* self,
    const VkMemoryRequirements* requirements,
    VkMemoryPropertyFlags properties,
    VkMemoryAllocateFlags flags,
    CuAllocation* allocation
);

/**
 * Frees the allocation.
 *
 * @param self The allocator (the `CuContext`).
 * @param allocation The allocation to free.
 */
void cuStandardAllocatorFreeCallback(void* self, CuAllocation* allocation);

/**
 * Standard allocation callbacks.
 */
const static CuAllocationCallbacks CU_STANDARD_ALLOCATOR_CALLBACKS = {
    .allocateCallback = cuStandardAllocatorAllocationCallback,
    .freeCallback = cuStandardAllocatorFreeCallback,
};
