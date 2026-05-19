#pragma once

#include "result.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * A window.
 */
typedef struct CuWindow {
    GLFWwindow* _window; /**< The GLFW window handle. */

    uint32_t _width; /**< The window's width. */

    uint32_t _height; /**< The window's height. */
} CuWindow;

/**
 * Creates the window.
 *
 * @param title The window's title.
 * @param width The window's width.
 * @param height The window's height.
 * @param window The window to initialize.
 * @return The result of the creation.
 */
CuResult cuCreateWindow(CuWindow* window, const char* title, uint32_t width, uint32_t height);

/**
 * Destroys and uninitializes the window.
 */
void cuDestroyWindow(CuWindow* window);

/** Updates the window. */
void cuUpdateWindow(CuWindow* window);

/**
 * @return The window's width.
 */
static inline uint32_t cuWindowWidth(const CuWindow* window) {
    return window->_width;
}

/**
 * @return The window's height.
 */
static inline uint32_t cuWindowHeight(const CuWindow* window) {
    return window->_height;
}

/**
 * @return Whether the window should clode.
 */
static inline bool cuShouldWindowClose(const CuWindow* window) {
    return glfwWindowShouldClose(window->_window);
}
