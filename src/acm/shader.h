#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>

namespace acm {

// A linked vertex + fragment shader program.
class Shader {
public:
    // Paths are relative to the repository's shaders/ directory.
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    void set(const char* name, float value) const;
    void set(const char* name, const glm::vec3& value) const;
    void set(const char* name, const glm::mat4& value) const;

    GLuint id() const { return program_; }

private:
    GLuint program_ = 0;
};

std::string shaderPath(const std::string& name);

}  // namespace acm
