#include "acm/capture.h"

#include <glad/gl.h>

#include <cstdio>
#include <vector>

namespace acm {

bool saveFramebufferPPM(const std::string& path, int width, int height) {
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) {
        return false;
    }
    std::fprintf(file, "P6\n%d %d\n255\n", width, height);
    // OpenGL's origin is bottom-left; image files expect top-left.
    for (int y = height - 1; y >= 0; --y) {
        std::fwrite(&pixels[static_cast<size_t>(y) * width * 3], 1, static_cast<size_t>(width) * 3, file);
    }
    return std::fclose(file) == 0;
}

}  // namespace acm
