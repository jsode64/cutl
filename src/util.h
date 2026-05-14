#pragma once

#include "result.h"

#include <stdint.h>

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
 * Wraps the `VkResult` into a `CuResult`.
 *
 * @param result The `VkResult` to wrap.
 *
 * @return The `VkResult` as a fitting `CuResult`.
 */
#define CU_VK_RESULT(result)                                                                       \
    ((CuResult){                                                                                   \
        .tag = CU_TAG_VK_ERROR,                                                                    \
        .val = (int32_t)result,                                                                    \
    })

#if defined(__APPLE__)
static const bool onApple = true;
#else
static const bool onApple = false;
#endif
