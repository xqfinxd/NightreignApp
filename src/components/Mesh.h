#pragma once
#include "public.h"
#include <string>

// Component for rendering mesh data
// Components only store resource names, not pointers
// The rendering system will look up resources from ResourceManager
struct MeshComponent
{
    std::string meshName;      // Name of mesh in ResourceManager
    std::string shaderName;    // Name of shader in ResourceManager
    std::string textureName;   // Name of texture in ResourceManager
    bool visible = true;

    MeshComponent() = default;
    
    MeshComponent(const std::string& mesh, const std::string& shader, const std::string& texture = "")
        : meshName(mesh), shaderName(shader), textureName(texture)
    {}
};

struct Vertex
{
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec4 color;

    Vertex()
        : position(0.0f)
        , texCoord(0.0f)
        , color(1.0f)
    {}

    Vertex(const glm::vec3& pos, const glm::vec2& uv, const glm::vec4& col = glm::vec4(1.0f))
        : position(pos)
        , texCoord(uv)
        , color(col)
    {}
};
