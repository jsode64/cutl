#pragma once

#include <stdint.h>

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The sum of the vectors.
 */
#define VEC4_ADD(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x + (b).x,                                                                        \
        .y = (a).y + (b).y,                                                                        \
        .z = (a).z + (b).z,                                                                        \
        .w = (a).w + (b).w,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The difference of the vectors.
 */
#define VEC4_SUB(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x - (b).x,                                                                        \
        .y = (a).y - (b).y,                                                                        \
        .z = (a).z - (b).z,                                                                        \
        .w = (a).w - (b).w,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The product of the vectors.
 */
#define VEC4_MUL(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x * (b).x,                                                                        \
        .y = (a).y * (b).y,                                                                        \
        .z = (a).z * (b).z,                                                                        \
        .w = (a).w * (b).w,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The quotient of the vectors.
 */
#define VEC4_DIV(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x / (b).x,                                                                        \
        .y = (a).y / (b).y,                                                                        \
        .z = (a).z / (b).z,                                                                        \
        .w = (a).w / (b).w,                                                                        \
    })

/**
 * A 4-dimensional vector of 64-bit floats.
 */
typedef struct vec4d {
    double x;
    double y;
    double z;
    double w;
} vec4d;

/**
 * A 4-dimensional vector of 32-bit floats.
 */
typedef struct vec4f {
    float x;
    float y;
    float z;
    float w;
} vec4f;

/**
 * A 4-dimensional vector of signed 32-bit integers.
 */
typedef struct vec4i {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t w;
} vec4i;

/**
 * A 4-dimensional vector if unsigned 32-bit integers.
 */
typedef struct vec4u {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t w;
} vec4u;
