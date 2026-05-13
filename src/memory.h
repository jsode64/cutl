#pragma once

#include <stddef.h>
#include <stdlib.h>

/**
 * Marks a pointer variable to be automatically freed with `cuAutoFreeFn`.
 */
#define CU_AUTO_FREE __attribute__((cleanup(cuAutoFreeFn)))

/**
 * Frees the pointed-to pointer.
 */
static inline void cuAutoFreeFn(void* p) {
    free(*(void**)p);
}

/**
 * A struct with a pointer to the given type and a size. Is freed automatically.
 */
#define CU_CHUNK_AUTO_FREE __attribute__((cleanup(cuAutoFreeChunkFn)))

/**
 * A null chunk.
 */
#define CU_CHUNK_NULL                                                                              \
    ((CuChunk){                                                                                    \
        ._n = 0,                                                                                   \
        ._data = NULL,                                                                             \
    })

/**
 * A chunk of memory with a count and data pointer.
 *
 * The type of `.data` depends on where the data comes from.
 */
typedef struct CuChunk {
    size_t _n; /**< The number of elements. */

    void* _data; /**< The data pointer. */
} CuChunk;

/**
 * Frees the chunk.
 */
static inline void cuAutoFreeChunkFn(const CuChunk* chunk) {
    free(chunk->_data);
}
