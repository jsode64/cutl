#pragma once

#include <stdint.h>

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The sum of the vectors.
 */
#define VEC3_ADD(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x + (b).x,                                                                        \
        .y = (a).y + (b).y,                                                                        \
        .z = (a).z + (b).z,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The difference of the vectors.
 */
#define VEC3_SUB(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x - (b).x,                                                                        \
        .y = (a).y - (b).y,                                                                        \
        .z = (a).z - (b).z,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The product of the vectors.
 */
#define VEC3_MUL(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x * (b).x,                                                                        \
        .y = (a).y * (b).y,                                                                        \
        .z = (a).z * (b).z,                                                                        \
    })

/**
 * @param a The left-hand side vector.
 * @param b The right-hand side vector.
 *
 * @return The quotient of the vectors.
 */
#define VEC3_DIV(a, b)                                                                             \
    ((typeof(a)){                                                                                  \
        .x = (a).x / (b).x,                                                                        \
        .y = (a).y / (b).y,                                                                        \
        .z = (a).z / (b).z,                                                                        \
    })

/**
 * A 3-dimensional vector of 64-bit floats.
 */
typedef struct vec3d {
    double x;
    double y;
    double z;
} vec3d;

/**
 * A 3-dimensional vector of 32-bit floats.
 */
typedef struct vec3f {
    float x;
    float y;
    float z;
} vec3f;

/**
 * A 3-dimensional vector of signed 32-bit integers.
 */
typedef struct vec3i {
    int32_t x;
    int32_t y;
    int32_t z;
} vec3i;

/**
 * A 3-dimensional vector if unsigned 32-bit integers.
 */
typedef struct vec3u {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} vec3u;