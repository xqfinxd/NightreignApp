#include "Shader.h"
#include <SDL_log.h>

Shader::Shader()
    : m_program(0)
{
}

Shader::Shader(uint32_t program, const std::string& vertPath, const std::string& fragPath)
    : m_program(program)
    , m_vertPath(vertPath)
    , m_fragPath(fragPath)
{
}

Shader::~Shader()
{
    // Note: Actual GPU resource cleanup should be handled by Device
    // This just releases the reference
}

void Shader::use() const
{
    if (m_program) {
        glUseProgram(m_program);
    }
}

void Shader::release()
{
    m_program = 0;
    m_vertPath.clear();
    m_fragPath.clear();
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
