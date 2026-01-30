#pragma once
#include "public.h"
#include <string>

class Shader
{
public:
    Shader();
    ~Shader();

    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath);
    void use() const;
    void cleanup();

    // Uniform setters
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec4(const std::string& name, const glm::vec4& vec) const;
    void setVec3(const std::string& name, const glm::vec3& vec) const;
    void setVec2(const std::string& name, const glm::vec2& vec) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;

    uint32_t getProgram() const { return m_program; }

private:
    uint32_t compileShader(const std::string& source, uint32_t type);
    bool linkProgram(uint32_t vertShader, uint32_t fragShader);
    std::string readFile(const std::string& path);

private:
    uint32_t m_program = 0;
};
