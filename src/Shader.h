#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

class Shader {
public:
    Shader(const std::string& vertPath, const std::string& fragPath) {
        GLuint vs = compile(GL_VERTEX_SHADER, vertPath);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fragPath);
        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        GLint ok; glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) { char log[1024]; glGetProgramInfoLog(id, 1024, nullptr, log); std::cerr << "Link error:\n" << log << "\n"; }
        glDeleteShader(vs); glDeleteShader(fs);
    }
    Shader(const Shader&) = delete;
    ~Shader() { glDeleteProgram(id); }

    void use() const { glUseProgram(id); }
    void setInt(const char* n, int v) const { glUniform1i(loc(n), v); }
    void setFloat(const char* n, float v) const { glUniform1f(loc(n), v); }
    void setVec3(const char* n, const glm::vec3& v) const { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
    void setMat3(const char* n, const glm::mat3& m) const { glUniformMatrix3fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void setMat4(const char* n, const glm::mat4& m) const { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }

private:
    GLuint id = 0;
    GLint loc(const char* n) const { return glGetUniformLocation(id, n); }

    static GLuint compile(GLenum type, const std::string& path) {
        std::ifstream f(path);
        if (!f) std::cerr << "Cannot open shader: " << path << "\n";
        std::stringstream ss; ss << f.rdbuf();
        std::string src = ss.str();
        const char* c = src.c_str();
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &c, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log); std::cerr << path << ":\n" << log << "\n"; }
        return s;
    }
};