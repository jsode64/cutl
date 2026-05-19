#pragma once

#include "allocation.h"
#include "context.h"
#include "renderer.h"
#include "result.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

/**
 * @param pBuffer A pointer to the buffer to get the device address of.
 *
 * @return The buffer's device address.
 */
#define CU_BUFFER_DEVICE_ADDRESS(buffer) ((buffer)->_address)

/**
 * A dynamic buffer with data that is written every frame.
 *
 * Contains one frame for every frame in flight in a renderer.
 */
typedef struct CuDynBuf {
    const CuRenderer* _renderer; /**< The source renderer. */

    CuAllocation _allocation; /**< The memory allocation. */

    VkBuffer _buffer; /**< The buffer handle. */

    uint64_t _size; /**< The size of one of the buffer's frames in bytes. */

    VkDeviceAddress _address; /**< The buffer's device address. */
} CuDynBuf;

/**
 * Creates the dynamic buffer.
 *
 * @param renderer The target renderer.
 * @param size The size of one of the buffer's frames.
 * @param allocator The allocator to use.
 * @param allocationCallbacks The allocator's allocation callbacks.
 * @param buffer The dynamic buffer to create.
 *
 * @return The result of the dynamic vertex buffer's creation.
 */
CuResult cuCreateDynBuf(
    const CuRenderer* renderer,
    uint64_t size,
    void* allocator,
    const CuAllocationCallbacks* allocationCallbacks,
    CuDynBuf* buffer
);

/**
 * Destroys the dynamic buffer.
 *
 * @param buffer The dynamic buffer to destroy.
 * @param allocator The source allocator.
 * @param allocationCallbacks The allocator's allocation callbacks.
 */
void cuDestroyDynBuf(
    CuDynBuf* buffer, void* allocator, const CuAllocationCallbacks* allocationCallbacks
);

/**
 * Writes the data to the dynamic buffer.
 *
 * @param buffer The dynamic buffer to write to.
 * @param size The size of the data to write.
 * @param data The data to write.
 *
 * @return The result of the data mapping and writing.
 */
CuResult cuDynBufWrite(CuDynBuf* buffer, uint64_t size, const void* data);

/**
 * Writes the data to all of the dynamic buffer's frames.
 *
 * @param buffer The dynamic buffer to write to.
 * @param size The size of the data to write.
 * @param data The data to write.
 *
 * @return The result of the data mapping and writing.
 *
 * @note This function should *not* be used during active rendering due to race conditions.
 */
CuResult cuDynBufWriteAll(CuDynBuf* buffer, uint64_t size, const void* data);
