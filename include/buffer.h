#pragma once

#include "allocation.h"
#include "context.h"
#include "renderer.h"
#include "result.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

/**
 * A dynamic vertex buffer with data that is written every frame.
 *
 * Contains one frame for every frame in flight in a renderer.
 */
typedef struct CuDynamicVbo {
    const CuRenderer* _renderer; /**< The source renderer. */

    CuAllocation _allocation; /**< The memory allocation. */

    VkBuffer _buffer; /**< The buffer handle. */

    uint64_t _size; /**< The size of one of the buffer's frames in bytes. */

    VkDeviceAddress _address; /**< The buffer's device address. */
} CuDynamicVbo;

/**
 * Creates the dynamic vertex buffer.
 *
 * @param renderer The target renderer.
 * @param size The size of one of the buffer's frames.
 * @param allocator The allocator to use.
 * @param allocationCallbacks The allocator's allocation callbacks.
 * @param vbo The dynamic vertex buffer to create.
 *
 * @return The result of the dynamic vertex buffer's creation.
 */
CuResult cuCreateDynamicVbo(
    const CuRenderer* renderer,
    uint64_t size,
    void* allocator,
    const CuAllocationCallbacks* allocationCallbacks,
    CuDynamicVbo* vbo
);

/**
 * Destroys the dynamic vertex buffer.
 *
 * @param vbo The dynamic vertex buffer to destroy.
 * @param allocator The source allocator..
 * @param allocationCallbacks The allocator's allocation callbacks.
 */
void cuDestroyDynamicVbo(
    CuDynamicVbo* vbo, void* allocator, const CuAllocationCallbacks* allocationCallbacks
);

/**
 * Writes the data to the dynamic vertex buffer.
 *
 * @param vbo The dynamic vertex buffer to write to.
 * @param size The size of the data to write.
 * @param data The data to write.
 *
 * @return The result of the data mapping and writing.
 */
CuResult cuDynamicVboWrite(CuDynamicVbo* vbo, uint64_t size, const void* data);

/**
 * Writes the data to all of the dynamic vertex buffer's frames.
 * 
 * @param vbo The dynamic vertex buffer to write to.
 * @param size The size of the data to write.
 * @param data The data to write.
 *
 * @return The result of the data mapping and writing.
 * 
 * @note This function should *not* be used during active rendering due to race conditions.
 */
CuResult cuDynamicVboWriteAll(CuDynamicVbo* vbo, uint64_t size, const void* data);

/**
 * Returns the dynamic vertex buffer's device address.
 * 
 * @param vbo The dynamic vertex buffer to get the device address of.
 * 
 * @return The dynamic vertex buffer's device address.
 */
static inline VkDeviceAddress cuDynamicVboDeviceAddress(const CuDynamicVbo* vbo) {
    return vbo->_address;
}
