#include "Shader.h"
#include <SDL_log.h>
#include <fstream>
#include <sstream>

Shader::Shader()
{
}

Shader::~Shader()
{
    cleanup();
}

bool Shader::loadFromFiles(const std::string& vertPath, const std::string& fragPath)
{
    std::string vertSource = readFile(vertPath);
    std::string fragSource = readFile(fragPath);

    if (vertSource.empty() || fragSource.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader: Failed to read shader files");
        return false;
    }

    uint32_t vertShader = compileShader(vertSource, GL_VERTEX_SHADER);
    if (vertShader == 0) return false;

    uint32_t fragShader = compileShader(fragSource, GL_FRAGMENT_SHADER);
    if (fragShader == 0) {
        glDeleteShader(vertShader);
        return false;
    }

    bool success = linkProgram(vertShader, fragShader);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    if (success) {
        SDL_Log("Shader: Loaded successfully (%s, %s)", vertPath.c_str(), fragPath.c_str());
    }

    return success;
}

void Shader::use() const
{
    if (m_program) {
        glUseProgram(m_program);
    }
}

void Shader::cleanup()
{
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    int location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
    }
}

void Shader::setVec4(const std::string& name, const glm::vec4& vec) const
{
    int location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {
        glUniform4fv(location, 1, &vec[0]);
    }
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const
{
    int location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {
        glUniform3fv(location, 1, &vec[0]);
    }
}

void Shader::setVec2(const std::string& name, const glm::vec2& vec) const
{
    int location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {
        glUniform2fv(location, 1, &vec[0]);
    }
}

void Shader::setFloat(const std::string& name, float value) const
{
    int location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void Shader::setInt(const std::string& name, int value) const
{
    int location = glGetUniformLocation(m_program, name.c_str());
    if (location != -1) {
        glUniform1i(location, value);
    }
}

uint32_t Shader::compileShader(const std::string& source, uint32_t type)
{
    uint32_t shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader: Compilation failed: %s", infoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool Shader::linkProgram(uint32_t vertShader, uint32_t fragShader)
{
    m_program = glCreateProgram();
    glAttachShader(m_program, vertShader);
    glAttachShader(m_program, fragShader);
    glLinkProgram(m_program);

    int success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader: Linking failed: %s", infoLog);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }

    return true;
}

std::string Shader::readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Shader: Failed to open file: %s", path.c_str());
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
