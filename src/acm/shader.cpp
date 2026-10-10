#include "acm/shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace acm {

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open shader file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compile(GLenum type, const std::string& path) {
    const std::string source = readFile(path);
    const char* text = source.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error("Failed to compile " + path + ":\n" + log);
    }
    return shader;
}

}  // namespace

std::string shaderPath(const std::string& name) {
    return std::string(ACM_SHADER_DIR) + "/" + name;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    GLuint vertex = compile(GL_VERTEX_SHADER, shaderPath(vertexPath));
    GLuint fragment = compile(GL_FRAGMENT_SHADER, shaderPath(fragmentPath));

    program_ = glCreateProgram();
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        glDeleteProgram(program_);
        throw std::runtime_error(std::string("Failed to link shader program:\n") + log);
    }
}

Shader::~Shader() {
    glDeleteProgram(program_);
}

void Shader::use() const {
    glUseProgram(program_);
}

void Shader::set(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(program_, name), value);
}

void Shader::set(const char* name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(program_, name), 1, glm::value_ptr(value));
}

void Shader::set(const char* name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(program_, name), 1, GL_FALSE, glm::value_ptr(value));
}

}  // namespace acm
