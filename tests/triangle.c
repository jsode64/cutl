#define CU_INCLUDE_MATH
#include "cutl.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>

int32_t main() {
    CuWindow window;
    CU_QUERY(cuCreateWindow(&window, "Triangle Test", 800, 800));

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
    CU_QUERY(cuReadShaderCode(&vertexCode, "tests/shaders/triangle.vert.spv"));
    CU_QUERY(cuReadShaderCode(&fragmentCode, "tests/shaders/triangle.frag.spv"));

    const CuPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
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

        CuScene scene = {};
        CU_QUERY(cuRendererBeginScene(&renderer, &scene));

        cuSceneBeginRender(&scene, &renderer);
        cuSceneBindPipeline(&scene, &pipeline);
        cuSceneDraw(&scene, 3, 1);
        cuSceneEndRender(&scene, &renderer);

        CU_QUERY(cuRendererSubmitScene(&renderer, &scene));
    }

    cuContextWaitIdle(&context);

    cuDestroyPipeline(&pipeline);
    cuDestroyPipelineLayout(&pipelineLayout);
    cuDestroyRenderer(&renderer);
    cuDestroyContext(&context);
    cuDestroyWindow(&window);

    return 0;
}
