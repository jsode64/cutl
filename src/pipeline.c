#include "pipeline.h"

#include "context.h"
#include "result.h"
#include "util.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

/**
 * An uninitialized pipeline layout.
 */
#define CU_NULL_PIPELINE_LAYOUT                                                                    \
    ((CuPipelineLayout){                                                                           \
        ._context = NULL,                                                                          \
        ._pipelineLayout = VK_NULL_HANDLE,                                                         \
    })

/**
 * An uninitialized shader code.
 */
#define CU_NULL_SHADER_CODE                                                                        \
    ((CuShaderCode){                                                                               \
        ._size = 0,                                                                                \
        ._code = NULL,                                                                             \
    })

/**
 * An uninitialized pipeline.
 */
#define CU_NULL_PIPELINE                                                                           \
    ((CuPipeline){                                                                                 \
        ._context = NULL,                                                                          \
        ._pipeline = VK_NULL_HANDLE,                                                               \
    })

CuResult cuCreatePipelineLayout(
    CuPipelineLayout* const pipelineLayout,
    const CuContext* const context,
    const CuPipelineLayoutCreateInfo* const info
) {
    *pipelineLayout = CU_NULL_PIPELINE_LAYOUT;
    pipelineLayout->_context = context;

    const VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = info->pushConstantSize,
    };
    const bool usePushConstants = info->pushConstantSize > 0;
    const VkPipelineLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = usePushConstants ? 1 : 0,
        .pPushConstantRanges = &pushConstantRange,
    };

    VkResult result = vkCreatePipelineLayout(
        context->_device, &createInfo, NULL, &pipelineLayout->_pipelineLayout
    );
    if (result != VK_SUCCESS) {
        *pipelineLayout = CU_NULL_PIPELINE_LAYOUT;
        CU_VK_ERROR(result);
    }

    return CU_SUCCESS;
}

void cuDestroyPipelineLayout(CuPipelineLayout* const pipelineLayout) {
    vkDestroyPipelineLayout(
        pipelineLayout->_context->_device, pipelineLayout->_pipelineLayout, NULL
    );

    *pipelineLayout = CU_NULL_PIPELINE_LAYOUT;
}

CuResult cuReadShaderCode(CuShaderCode* const shaderCode, const char* const path) {
    *shaderCode = CU_NULL_SHADER_CODE;

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        goto FAIL;
    }

    // Get the file's length.
    if (fseek(file, 0, SEEK_END) != 0) {
        goto FAIL;
    }
    const size_t size = (size_t)ftell(file);
    if (fseek(file, 0, SEEK_SET) != 0) {
        goto FAIL;
    }

    // Validate the length and allocate the buffer.
    if (size % 4 != 0) {
        goto FAIL;
    }
    shaderCode->_size = size;
    shaderCode->_code = malloc(size);
    if (shaderCode->_code == NULL) {
        goto FAIL;
    }

    // Read the code into the buffer.
    if (fread((void*)shaderCode->_code, 1, size, file) != size) {
        goto FAIL;
    }

    fclose(file);

    return CU_SUCCESS;

FAIL:
    fclose(file);
    cuDestroyShaderCode(shaderCode);
    return CU_STD_ERROR;
}

void cuDestroyShaderCode(CuShaderCode* const shaderCode) {
    free(shaderCode->_code);

    *shaderCode = CU_NULL_SHADER_CODE;
}

CuResult cuCreatePipeline(
    CuPipeline* const pipeline,
    const CuContext* const context,
    const CuPipelineLayout* const pipelineLayout,
    const CuShaderCode* const vertexCode,
    const CuShaderCode* const fragmentCode,
    const CuPipelineCreateInfo* const info
) {
    *pipeline = CU_NULL_PIPELINE;
    pipeline->_context = context;

    const char* shaderMainFnName = "main";
    const VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = vertexCode->_size,
        .pCode = vertexCode->_code,
    };
    const VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = fragmentCode->_size,
        .pCode = fragmentCode->_code,
    };
    const VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = &vertexShaderModuleCreateInfo,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = VK_NULL_HANDLE,
            .pName = shaderMainFnName,
            .pSpecializationInfo = NULL,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = &fragmentShaderModuleCreateInfo,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = VK_NULL_HANDLE,
            .pName = shaderMainFnName,
            .pSpecializationInfo = NULL,
        },
    };
    const VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .topology = info->topology,
        .primitiveRestartEnable = VK_FALSE,
    };
    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = 0.0f,
        .height = 0.0f,
        .minDepth = 0.0f,
        .maxDepth = 0.0f,
    };
    const VkRect2D scissor = {
        .offset =
            {
                .x = 0,
                .y = 0,
            },
        .extent = {
            .width = 0,
            .height = 0,
        },
    };
    const VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };
    const VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = info->polygonMode,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };
    const VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_NEVER,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };
    const VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    const VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = colorWriteMask,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };
    const VkPipelineRenderingCreateInfo renderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
        .viewMask = 0,
        .colorAttachmentCount = info->nFormats,
        .pColorAttachmentFormats = info->formats,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };
    const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]),
        .pDynamicStates = dynamicStates,
    };
    const VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCreateInfo,
        .flags = 0,
        .stageCount = sizeof(shaderStageCreateInfos) / sizeof(shaderStageCreateInfos[0]),
        .pStages = shaderStageCreateInfos,
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyCreateInfo,
        .pTessellationState = NULL,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = pipelineLayout->_pipelineLayout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VkResult result = vkCreateGraphicsPipelines(
        context->_device, VK_NULL_HANDLE, 1, &createInfo, NULL, &pipeline->_pipeline
    );
    if (result != VK_SUCCESS) {
        *pipeline = CU_NULL_PIPELINE;
        return CU_VK_ERROR(result);
    }

    return CU_SUCCESS;
}

void cuDestroyPipeline(CuPipeline* const pipeline) {
    vkDestroyPipeline(pipeline->_context->_device, pipeline->_pipeline, NULL);

    *pipeline = CU_NULL_PIPELINE;
}
