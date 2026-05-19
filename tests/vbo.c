#define CU_INCLUDE_MATH
#include "cutl.h"
#include "util.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @return A vertex with the given position and color.
 */
#define VERTEX(_x, _y, _r, _g, _b)                                                                 \
    ((Vertex){                                                                                     \
        {_x, _y},                                                                                  \
        {_r, _g, _b, 1.0f},                                                                        \
    })

/**
 * A vertex for the VBO test shader.
 */
typedef struct Vertex {
    vec2f p;
    vec4f c;
} Vertex;

int32_t main() {
    Vertex vertices[] = {
        // The bottom triangle.
        VERTEX(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
        VERTEX(1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
        VERTEX(0.0f, 0.0f, 0.5f, 0.5f, 0.5f),

        // The left triangle.
        VERTEX(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
        VERTEX(-1.0f, -1.0f, 0.0f, 0.0f, 0.0f),
        VERTEX(0.0f, 0.0f, 0.5f, 0.5f, 0.5f),

        // The top triangle.
        VERTEX(-1.0f, -1.0f, 0.0f, 0.0f, 0.0f),
        VERTEX(1.0f, -1.0f, 0.0f, 0.0f, 0.0f),
        VERTEX(0.0f, 0.0f, 0.5f, 0.5f, 0.5f),

        // The right triangle.
        VERTEX(1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
        VERTEX(1.0f, -1.0f, 0.0f, 0.0f, 0.0f),
        VERTEX(0.0f, 0.0f, 0.5f, 0.5f, 0.5f),
    };
    const uint32_t nVertices = sizeof(vertices) / sizeof(vertices[0]);

    CuWindow window;
    CU_QUERY(cuCreateWindow(&window, "VBO Test", 800, 800));

    const CuContextCreateInfo contextCreateInfo = {
        .window = &window,
        .useValidation = true,
    };
    CuContext context;
    CU_QUERY(cuCreateContext(&context, &contextCreateInfo));

    const CuRendererCreateInfo rendererCreateInfo = {
        .nFrames = 3,
    };
    CuRenderer renderer;
    CU_QUERY(cuCreateRenderer(&renderer, &context, &rendererCreateInfo));

    CuShaderCode vertexCode = {};
    CuShaderCode fragmentCode = {};
    CU_QUERY(cuReadShaderCode(&vertexCode, "tests/shaders/vbo.vert.spv"));
    CU_QUERY(cuReadShaderCode(&fragmentCode, "tests/shaders/vbo.frag.spv"));

    CuDynBuf vbo = {};
    const uint64_t vertexDataSize = sizeof(vertices);
    CU_QUERY(
        cuCreateDynBuf(&renderer, vertexDataSize, &context, &CU_STANDARD_ALLOCATOR_CALLBACKS, &vbo)
    );
    CU_QUERY(cuDynBufWriteAll(&vbo, vertexDataSize, vertices));

    const CuPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .pushConstantSize = 8,
    };
    CuPipelineLayout pipelineLayout = {};
    CU_QUERY(cuCreatePipelineLayout(&pipelineLayout, &context, &pipelineLayoutCreateInfo));

    VkFormat rendererFormat = cuRendererFormat(&renderer);
    const CuPipelineCreateInfo pipelineCreateInfo = {
        .polygonMode = VK_POLYGON_MODE_FILL,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .nFormats = 1,
        .formats = &rendererFormat,
    };
    CuPipeline pipeline = {};
    CU_QUERY(cuCreatePipeline(
        &pipeline, &context, &pipelineLayout, &vertexCode, &fragmentCode, &pipelineCreateInfo
    ));

    cuDestroyShaderCode(&vertexCode);
    cuDestroyShaderCode(&fragmentCode);

    while (!cuShouldWindowClose(&window)) {
        cuUpdateWindow(&window);

        // Set the triangle tips' positions.
        const double t = 10.0 * (double)clock() / (double)CLOCKS_PER_SEC;
        const float x = (float)cos(t);
        const float y = (float)sin(t);
        for (uint32_t i = 2; i < nVertices; i += 3) {
            Vertex* const v = &vertices[i];
            v->p = (vec2f){x, y};
        }
        cuDynBufWrite(&vbo, sizeof(vertices), vertices);

        CuScene scene = {};
        CU_QUERY(cuRendererBeginScene(&renderer, &scene));

        cuSceneBeginRender(&scene, &renderer);
        cuSceneBindPipeline(&scene, &pipeline);

        VkDeviceAddress bufferAddress = CU_BUFFER_DEVICE_ADDRESS(&vbo);
        cuSceneSetPushConstants(&scene, &pipelineLayout, 8, &bufferAddress);

        cuSceneDraw(&scene, nVertices, 1);
        cuSceneEndRender(&scene, &renderer);

        CU_QUERY(cuRendererSubmitScene(&renderer, &scene));
    }

    cuContextWaitIdle(&context);

    cuDestroyDynBuf(&vbo, &context, &CU_STANDARD_ALLOCATOR_CALLBACKS);
    cuDestroyPipeline(&pipeline);
    cuDestroyPipelineLayout(&pipelineLayout);
    cuDestroyRenderer(&renderer);
    cuDestroyContext(&context);
    cuDestroyWindow(&window);

    return 0;
}
