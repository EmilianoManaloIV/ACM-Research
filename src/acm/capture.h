#pragma once

#include <string>

namespace acm {

// Saves the current framebuffer as a binary PPM image, so experiments can
// record reproducible figures. Returns false if the file cannot be written.
bool saveFramebufferPPM(const std::string& path, int width, int height);

}  // namespace acm
