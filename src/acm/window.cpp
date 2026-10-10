#include "acm/window.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <stdexcept>

namespace acm {

namespace {

void errorCallback(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

}  // namespace

Window::Window(int width, int height, const std::string& title, bool visible) {
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // required on macOS
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create an OpenGL 3.3 window");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress)) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("Failed to load OpenGL functions");
    }
}

Window::~Window() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void Window::swapAndPoll() {
    glfwSwapBuffers(window_);
    glfwPollEvents();
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

void Window::framebufferSize(int& width, int& height) const {
    glfwGetFramebufferSize(window_, &width, &height);
}

double Window::time() const {
    return glfwGetTime();
}

}  // namespace acm
