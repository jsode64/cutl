#include "buffer.h"
#include "context.h"
#include "pipeline.h"
#include "renderer.h"
#include "result.h"
#include "scene.h"
#include "util.h"
#include "window.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @return A vertex with the given position and color.
 */
#define VERTEX(_x, _y, _r, _g, _b) ((Vertex){ .x = _x, .y = _y, .r = _r, .g = _g, .b = _b, .a = 1.0f })

/**
 * A vertex for the VBO test shader.
 */
typedef struct Vertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
} Vertex;

/**
 * @return A random valid vertex.
 */
Vertex randVertex();

/**
 * @return A random float from -1 to 1 inclusive.
 */
float randFloat();

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
    CU_QUERY(cuCreateWindow(&window, "Hello", 1600, 900));

    const CuContextCreateInfo contextCreateInfo = {
        .useValidation = true,
        .window = &window,
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

    CuDynamicVbo dynamicVbo = {};
    const uint64_t vertexDataSize = sizeof(vertices);
    CU_QUERY(cuCreateDynamicVbo(
        &renderer, vertexDataSize, &context, &CU_STANDARD_ALLOCATOR_CALLBACKS, &dynamicVbo
    ));
    CU_QUERY(cuDynamicVboWriteAll(&dynamicVbo, vertexDataSize, vertices));

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
        const double t = (double)clock() / (double)CLOCKS_PER_SEC;
        const float x = (float)cos(t);
        const float y = (float)sin(t);
        for (uint32_t i = 2; i < nVertices; i += 3) {
            Vertex* const v = &vertices[i];
            v->x = x;
            v->y = y;
        }
        cuDynamicVboWrite(&dynamicVbo, sizeof(vertices), vertices);

        CuScene scene = {};
        CU_QUERY(cuRendererBeginScene(&renderer, &scene));

        cuSceneBeginRender(&scene, &renderer);
        cuSceneBindPipeline(&scene, &pipeline);

        VkDeviceAddress bufferAddress = cuDynamicVboDeviceAddress(&dynamicVbo);
        cuSceneSetPushConstants(&scene, &pipelineLayout, 8, &bufferAddress);

        cuSceneDraw(&scene, nVertices, 1);
        cuSceneEndRender(&scene, &renderer);

        CU_QUERY(cuRendererSubmitScene(&renderer, &scene));
    }

    cuContextWaitIdle(&context);

    cuDestroyDynamicVbo(&dynamicVbo, &context, &CU_STANDARD_ALLOCATOR_CALLBACKS);
    cuDestroyPipeline(&pipeline);
    cuDestroyPipelineLayout(&pipelineLayout);
    cuDestroyRenderer(&renderer);
    cuDestroyContext(&context);
    cuDestroyWindow(&window);

    return 0;
}

Vertex randVertex() {
    return (Vertex){
        .x = randFloat(),
        .y = randFloat(),
        .r = fabsf(randFloat()),
        .g = fabsf(randFloat()),
        .b = fabsf(randFloat()),
        .a = 1.0f,
    };
}

float randFloat() {
    return (2.0f * (float)rand() / (float)RAND_MAX) - 1.0f;
}
