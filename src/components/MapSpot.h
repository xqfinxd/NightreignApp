#pragma once
#include "public.h"
#include <string>

// Component for map spots/markers
struct MapSpot
{
    static float textScale;      // Adjust this to change text size
    glm::vec2 gridPosition;      // Position in grid coordinates (x, y)
    std::string textureName;     // Texture to use for this spot
    float size = 0.2f;           // Size of the spot in world units
    glm::vec4 tint = glm::vec4(1.0f);  // Color tint
    std::string metadata;        // Optional metadata for identifying this spot
    std::string label;           // Text label to display below spot
    
    MapSpot() = default;
    
    MapSpot(const glm::vec2& pos, const std::string& texture = "spot", float sz = 0.2f)
        : gridPosition(pos)
        , textureName(texture)
        , size(sz)
    {}
};
