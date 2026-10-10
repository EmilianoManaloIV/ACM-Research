#pragma once

#include <string>

struct GLFWwindow;

namespace acm {

// Owns a GLFW window with an OpenGL 3.3 core context.
class Window {
public:
    Window(int width, int height, const std::string& title, bool visible = true);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void swapAndPoll();
    void framebufferSize(int& width, int& height) const;
    double time() const;

    GLFWwindow* handle() const { return window_; }

private:
    GLFWwindow* window_ = nullptr;
};

}  // namespace acm
