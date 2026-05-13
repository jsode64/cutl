#pragma once

#include "context.h"
#include "result.h"

#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

/**
 * Create info for a pipeline layout.
 */
typedef struct CuPipelineLayoutCreateInfo {
    uint64_t pushConstantSize; /**< The size of the push constant data. */
} CuPipelineLayoutCreateInfo;

/**
 * A pipeline layout.
 */
typedef struct CuPipelineLayout {
    const CuContext* _context; /**< The source context. */

    VkPipelineLayout _pipelineLayout; /**< The pipeline layout handle. */
} CuPipelineLayout;

/**
 * Shader code.
 */
typedef struct CuShaderCode {
    size_t _size; /**< The number of SPIRV words in the code. */

    uint32_t* _code; /**< The SPIRV code. */
} CuShaderCode;

/**
 * Create info for a pipeline.
 */
typedef struct CuPipelineCreateInfo {
    VkPolygonMode polygonMode; /**< The rasterization polygon mode. */

    VkPrimitiveTopology topology; /**< The primitive topology type. */

    uint32_t nFormats; /**< The number of formats the pipeline will support. */

    VkFormat* formats; /**< The formats the pipeline will support. */
} CuPipelineCreateInfo;

/**
 * A pipeline.
 */
typedef struct CuPipeline {
    const CuContext* _context; /**< The source context. */

    VkPipeline _pipeline; /**< The pipeline handle. */
} CuPipeline;

/**
 * Creates the pipeline layout.
 *
 * @param pipelineLayout The pipeline layout to create.
 * @param context The source context.
 * @param info The pipeline layout create info.
 *
 * @return The result of the pipeline layout creation.
 */
CuResult cuCreatePipelineLayout(
    CuPipelineLayout* pipelineLayout,
    const CuContext* context,
    const CuPipelineLayoutCreateInfo* info
);

/**
 * Destroys the pipeline layout.
 *
 * @param pipelineLayout The pipeline layout to destroy.
 */
void cuDestroyPipelineLayout(CuPipelineLayout* pipelineLayout);

/**
 * Reads the SPIRV code file into the shader code.
 *
 * @param shaderCode The shader code to be read into.
 * @param path The path to the SPIRV code file.
 *
 * @return The result of the code reading.
 */
CuResult cuReadShaderCode(CuShaderCode* shaderCode, const char* path);

/**
 * Destroys the shader code.
 *
 * @param shaderCode The shader code to destroy.
 */
void cuDestroyShaderCode(CuShaderCode* shaderCode);

/**
 * Creates a pipeline from the create info.
 *
 * @param pipeline The pipeline to create.
 * @param context The source context.
 * @param pipelineLayout The source pipeline layout.
 * @param vertexCode The vertex shader code.
 * @param fragmentcode The fragment shader code.
 * @param info The pipeline's create info.
 *
 * @return The result of the pipeline creation.
 */
CuResult cuCreatePipeline(
    CuPipeline* pipeline,
    const CuContext* context,
    const CuPipelineLayout* pipelineLayout,
    const CuShaderCode* vertexCode,
    const CuShaderCode* fragmentCode,
    const CuPipelineCreateInfo* info
);

/**
 * Destroys the pipeline.
 *
 * @param pipeline The pipeline to destroy.
 */
void cuDestroyPipeline(CuPipeline* pipeline);
