#pragma once

#include "result.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @return The smaller of the two values.
 */
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

/**
 * @return The larger of the two values.
 */
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

/**
 * Clamps the value within the ranges.
 *
 * @param v The value to be clamped.
 * @param min The minimum value.
 * @param max The maximum value.
 *
 * @return The value clamped between the minimum and maximum.
 */
#define CLAMP(v, min, max) MAX(min, MIN(v, max))

/**
 * @param _val The Cutl error value.
 *
 * @return A `CU_RESULT` for the Cutl error.
 */
#define CU_ERROR(_val) ((CuResult){.tag = CU_TAG_CU_ERROR, .val = (_val)})

/**
 * @return A `CU_RESULT` for a `libc` operation failure using `errno`.
 */
#define CU_STD_ERROR ((CuResult){.tag = CU_TAG_STD_ERROR, .val = errno})

/**
 * @param _val The Vulkan error value.
 *
 * @return A `CU_RESULT` for the Vulkan error.
 */
#define CU_VK_ERROR(_val) ((CuResult){.tag = CU_TAG_VK_ERROR, .val = (_val)})

/**
 * @return A `CU_RESULT` for a GLFW operation failure using `glfwGetError`.
 */
#define CU_GLFW_ERROR ((CuResult){.tag = CU_TAG_GLFW_ERROR, .val = glfwGetError(NULL)})

#if defined(__APPLE__)
#define CU_TARGET_OS (CU_OS_APPLE)
#elif defined(__linux__)
#define CU_TARGET_OS (CU_OS_LINUX)
#elif defined(_WIN32)
#define CU_TARGET_OS (CU_OS_WINDOWS)
#else
#error "Unsupported operating system"
#endif

#define CU_ON_APPLE (CU_TARGET_OS == CU_OS_APPLE)
#define CU_ON_LINUX (CU_TARGET_OS == CU_OS_LINUX)
#define CU_ON_WINDOWS (CU_TARGET_OS == CU_OS_WINDOWS)

/**
 * Marks a pointer variable to be automatically freed with `cuAutoFreeFn`.
 */
#define CU_AUTO_FREE __attribute__((cleanup(cuAutoFreeFn)))

/**
 * A struct with a pointer to the given type and a size. Is freed automatically.
 */
#define CU_CHUNK_AUTO_FREE __attribute__((cleanup(cuDestroyChunk)))

/**
 * @param n The number of elements to copy from the array.
 * @param arr The array to copy from.
 *
 * @return A chunk from the array, or `CU_NULL_CHUNK` on failure.
 */
#define CU_CHUNK_FROM_ARRAY(n, arr) cuCreateChunk(sizeof((arr)[0]), n, (arr))

/**
 * A null chunk.
 */
#define CU_NULL_CHUNK                                                                              \
    ((CuChunk){                                                                                    \
        .n = 0,                                                                                    \
        .data = NULL,                                                                              \
    })

/**
 * An OS target.
 */
typedef enum CuTargetOs {
    CU_OS_APPLE = 1, /**< Any Apple OS. */

    CU_OS_LINUX, /**< Any Linux distribution. */

    CU_OS_WINDOWS, /**< Windows. */
} CuTargetOs;

/**
 * A chunk of memory with a count and data pointer.
 *
 * The type of `.data` depends on where the data comes from.
 */
typedef struct CuChunk {
    size_t n; /**< The number of elements. */

    void* data; /**< The data pointer. */
} CuChunk;

/**
 * Creates the cunk from the given data.
 *
 * @param z The size of the data type being stored.
 * @param n The number of elements to copy.
 * @param p A pointer to the data to copy.
 *
 * @return The data copied into a chunk, or `CU_NULL_CHUNK` on failure.
 */
static inline CuChunk cuCreateChunk(const size_t z, const size_t n, const void* const p) {
    const size_t dataSize = n * z;
    void* data = NULL;
    if (dataSize == 0 || (data = malloc(dataSize)) == NULL) {
        return CU_NULL_CHUNK;
    }

    memcpy(data, p, dataSize);

    return (CuChunk){
        .n = n,
        .data = data,
    };
}

/**
 * Frees the chunk.
 */
static inline void cuDestroyChunk(CuChunk* chunk) {
    free(chunk->data);

    *chunk = CU_NULL_CHUNK;
}

/**
 * Frees the pointer.
 */
static inline void cuAutoFreeFn(void* p) {
    free(*(void**)p);
}
