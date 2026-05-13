#pragma once

#include "result.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * If the resuslt is an error, prints it an exits.
 */
#define CU_QUERY(result)                                                                           \
    gResult = (result);                                                                            \
    if (gResult.tag != CU_TAG_SUCCESS) {                                                           \
        printf("Error: `.tag`=%i, `.val`=%i\n", gResult.tag, gResult.val);                         \
        exit(1);                                                                                   \
    }

/**
 * The global result for `CU_QUERY`.
 */
static CuResult gResult;
