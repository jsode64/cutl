#pragma once

#include <stdint.h>

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The sum of the vectors.
 */
#define VEC2_ADD(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x + (b).x,                                                                        \
        .y = (a).y + (b).y,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The difference of the vectors.
 */
#define VEC2_SUB(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x - (b).x,                                                                        \
        .y = (a).y - (b).y,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The product of the vectors.
 */
#define VEC2_MUL(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x * (b).x,                                                                        \
        .y = (a).y * (b).y,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The quotient of the vectors.
 */
#define VEC2_DIV(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x / (b).x,                                                                        \
        .y = (a).y / (b).y,                                                                        \
    })

/**
 * A 2-dimensional vector of 64-bit floats.
 */
typedef struct vec2d {
    double x;
    double y;
} vec2d;

/**
 * A 2-dimensional vector of 32-bit floats.
 */
typedef struct vec2f {
    float x;
    float y;
} vec2f;

/**
 * A 2-dimensional vector of signed 32-bit integers.
 */
typedef struct vec2i {
    int32_t x;
    int32_t y;
} vec2i;

/**
 * A 2-dimensional vector if unsigned 32-bit integers.
 */
typedef struct vec2u {
    uint32_t x;
    uint32_t y;
} vec2u;
