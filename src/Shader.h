#pragma once
#include "public.h"
#include <string>

class Shader
{
public:
    Shader();
    Shader(uint32_t program, const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    void use() const;

    // Uniform setters
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec4(const std::string& name, const glm::vec4& vec) const;
    void setVec3(const std::string& name, const glm::vec3& vec) const;
    void setVec2(const std::string& name, const glm::vec2& vec) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;

    // Getters
    uint32_t getProgram() const { return m_program; }
    const std::string& getVertexPath() const { return m_vertPath; }
    const std::string& getFragmentPath() const { return m_fragPath; }
    bool isValid() const { return m_program != 0; }

    // Resource management
    void release();

private:
    uint32_t m_program = 0;
    std::string m_vertPath;
    std::string m_fragPath;
};
