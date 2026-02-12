#pragma once
#include "public.h"
#include <string>
#include <any>
#include <glm/glm.hpp>

// Component for map spots/markers
struct MapSpot
{
    static float textScale;             // Adjust this to change text size
    glm::vec2 gridPosition{-1.f, -1.f}; // Position in grid coordinates (x, y)
    std::string textureName;            // Texture to use for this spot
    glm::vec4 tint{ 1.f };              // Color tint
    int spotId = -1;                    // Spot ID from GameData
    std::string label;                  // Text label to display below spot
    
    MapSpot() = default;
    
    MapSpot(const glm::vec2& pos, const std::string& texture = "spot")
        : gridPosition(pos)
        , textureName(texture)
    {}
};
