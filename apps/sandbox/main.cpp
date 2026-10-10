// Sandbox: a rotating, Blinn-Phong-lit cube. Copy this app as the starting
// point for a new rendering experiment.
//
// Usage:
//   sandbox                                  interactive window (Esc quits)
//   sandbox --frames 60 --screenshot out.ppm render 60 frames, save the last one
//   sandbox --hidden ...                     no visible window (with --frames)

#include "acm/capture.h"
#include "acm/shader.h"
#include "acm/window.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

namespace {

struct Options {
    int frames = 0;  // 0 = run until the window is closed
    std::string screenshot;
    bool hidden = false;
};

Options parseArgs(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            options.frames = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            options.screenshot = argv[++i];
        } else if (std::strcmp(argv[i], "--hidden") == 0) {
            options.hidden = true;
        } else {
            std::fprintf(stderr, "usage: %s [--frames N] [--screenshot file.ppm] [--hidden]\n", argv[0]);
            std::exit(2);
        }
    }
    return options;
}

// 36 vertices: position (xyz) followed by normal (xyz), two triangles per face.
const float kCube[] = {
    // -Z
    -0.5f, -0.5f, -0.5f, 0, 0, -1,  0.5f,  0.5f, -0.5f, 0, 0, -1,  0.5f, -0.5f, -0.5f, 0, 0, -1,
     0.5f,  0.5f, -0.5f, 0, 0, -1, -0.5f, -0.5f, -0.5f, 0, 0, -1, -0.5f,  0.5f, -0.5f, 0, 0, -1,
    // +Z
    -0.5f, -0.5f,  0.5f, 0, 0, 1,   0.5f, -0.5f,  0.5f, 0, 0, 1,   0.5f,  0.5f,  0.5f, 0, 0, 1,
     0.5f,  0.5f,  0.5f, 0, 0, 1,  -0.5f,  0.5f,  0.5f, 0, 0, 1,  -0.5f, -0.5f,  0.5f, 0, 0, 1,
    // -X
    -0.5f,  0.5f,  0.5f, -1, 0, 0, -0.5f,  0.5f, -0.5f, -1, 0, 0, -0.5f, -0.5f, -0.5f, -1, 0, 0,
    -0.5f, -0.5f, -0.5f, -1, 0, 0, -0.5f, -0.5f,  0.5f, -1, 0, 0, -0.5f,  0.5f,  0.5f, -1, 0, 0,
    // +X
     0.5f,  0.5f,  0.5f, 1, 0, 0,   0.5f, -0.5f, -0.5f, 1, 0, 0,   0.5f,  0.5f, -0.5f, 1, 0, 0,
     0.5f, -0.5f, -0.5f, 1, 0, 0,   0.5f,  0.5f,  0.5f, 1, 0, 0,   0.5f, -0.5f,  0.5f, 1, 0, 0,
    // -Y
    -0.5f, -0.5f, -0.5f, 0, -1, 0,  0.5f, -0.5f, -0.5f, 0, -1, 0,  0.5f, -0.5f,  0.5f, 0, -1, 0,
     0.5f, -0.5f,  0.5f, 0, -1, 0, -0.5f, -0.5f,  0.5f, 0, -1, 0, -0.5f, -0.5f, -0.5f, 0, -1, 0,
    // +Y
    -0.5f,  0.5f, -0.5f, 0, 1, 0,   0.5f,  0.5f,  0.5f, 0, 1, 0,   0.5f,  0.5f, -0.5f, 0, 1, 0,
     0.5f,  0.5f,  0.5f, 0, 1, 0,  -0.5f,  0.5f, -0.5f, 0, 1, 0,  -0.5f,  0.5f,  0.5f, 0, 1, 0,
};

}  // namespace

int main(int argc, char** argv) {
    const Options options = parseArgs(argc, argv);

    try {
        acm::Window window(1280, 720, "ACM Research Sandbox", !options.hidden);
        acm::Shader shader("basic.vert", "basic.frag");

        GLuint vao = 0;
        GLuint vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(kCube), kCube, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                              reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        const glm::vec3 cameraPos(0.0f, 1.2f, 3.0f);
        const glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        int width = 0;
        int height = 0;
        for (int frame = 0; !window.shouldClose(); ++frame) {
            window.framebufferSize(width, height);
            glViewport(0, 0, width, height);
            glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Fixed timestep in batch mode keeps screenshots reproducible.
            const float t = options.frames > 0 ? frame / 60.0f : static_cast<float>(window.time());
            const float aspect = height > 0 ? static_cast<float>(width) / height : 1.0f;
            const glm::mat4 model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.3f, 1.0f, 0.1f));

            shader.use();
            shader.set("uModel", model);
            shader.set("uView", view);
            shader.set("uProjection", glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f));
            shader.set("uLightDir", glm::vec3(-0.5f, -1.0f, -0.6f));
            shader.set("uCameraPos", cameraPos);
            shader.set("uColor", glm::vec3(0.85f, 0.45f, 0.25f));

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            const bool lastFrame = options.frames > 0 && frame + 1 >= options.frames;
            if (lastFrame && !options.screenshot.empty()) {
                if (!acm::saveFramebufferPPM(options.screenshot, width, height)) {
                    std::fprintf(stderr, "Could not write %s\n", options.screenshot.c_str());
                    return 1;
                }
                std::printf("Saved %s (%dx%d)\n", options.screenshot.c_str(), width, height);
            }
            window.swapAndPoll();
            if (lastFrame) {
                break;
            }
        }

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    return 0;
}
