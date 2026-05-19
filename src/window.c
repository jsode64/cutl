#include "window.h"

#include "result.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdlib.h>

#define CU_NULL_WINDOW                                                                             \
    ((CuWindow){                                                                                   \
        ._window = NULL,                                                                           \
        ._width = 0,                                                                               \
        ._height = 0,                                                                              \
    })

CuResult cuCreateWindow(CuWindow* window, const char* title, uint32_t width, uint32_t height) {
    *window = CU_NULL_WINDOW;

    // Initialize GLFW.
    if (glfwInit() != GLFW_TRUE) {
        goto FAIL;
    }

    // Create window.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window->_window = glfwCreateWindow((int)width, (int)height, title, NULL, NULL);
    if (window->_window == NULL) {
        goto FAIL;
    }

    cuUpdateWindow(window);

    return CU_SUCCESS;

FAIL:
    cuDestroyWindow(window);
    return (CuResult){
        .tag = CU_TAG_GLFW_ERROR,
        .val = glfwGetError(NULL),
    };
}

void cuDestroyWindow(CuWindow* window) {
    glfwDestroyWindow(window->_window);

    *window = CU_NULL_WINDOW;
}

void cuUpdateWindow(CuWindow* window) {
    glfwPollEvents();

    int width = 0, height = 0;
    glfwGetFramebufferSize(window->_window, &width, &height);
    window->_width = (uint32_t)width;
    window->_height = (uint32_t)height;
}
