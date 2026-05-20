#pragma once

#include <stdint.h>

/**
 * A successful operation.
 */
#define CU_SUCCESS ((CuResult){.tag = CU_TAG_SUCCESS, .val = 0})

/**
 * @param result The result to query.
 *
 * @return Whether the result is a success value.
 */
#define CU_IS_SUCCESS(result) ((result).tag == CU_TAG_SUCCESS)

/**
 * An error type tag.
 *
 * Used as a tag for `CuResult`.
 */
typedef enum CuResultTag {
    /**
     * A success.
     * The `.val` should be ignored.
     */
    CU_TAG_SUCCESS = 0,

    /**
     * A general error.
     * The `.val` is the error's `CuError`.
     */
    CU_TAG_CU_ERROR,

    /**
     * A C standard library error.
     * The `.val` is the value of `errno`.
     */
    CU_TAG_STD_ERROR,

    /**
     * A Vulkan error.
     * The `.val` is the `VkResult` value.
     */
    CU_TAG_VK_ERROR,

    /**
     * A GLFW error.
     * The `.val` is the return value of `glfwGetError`.
     */
    CU_TAG_GLFW_ERROR,
} CuResultTag;

/**
 * A Cutl error value.
 *
 * Used as the `.val` for `CuResult`s where `.tag` is `CU_TAG_
 */
typedef enum CuError {
    /**
     * Out of memory. An allocation failed.
     */
    CU_ERROR_OUT_OF_MEMORY,
} CuError;

/**
 * A Cuttle operation result.
 *
 * If `.tag` is not `CU_TAG_SUCCESS`, it is an error.
 */
typedef struct CuResult {
    /** The error type. Tells whether the result is a success and what `.val` holds. */
    CuResultTag tag;

    /** The error value. Meaning depends on what `.tag` is. See `CuResultTag` for details. */
    int32_t val;
} CuResult;
