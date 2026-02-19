#pragma once
#include "public.h"
#include <string>
#include <any>
#include <glm/glm.hpp>

// Component for map spots/markers
struct MapSpot
{
    static float textScale;             // Adjust this to change text size
    static float iconSize;              // Base icon size
    int metadata = -1;                  // Spot ID from GameData
    std::string label;                  // Text label to display below spot
    bool visible = true;                // Is this spot visible
    bool selected = false;              // Is this spot selected
    int alignment = 0;                  // 0=center, 1=top, 2=bottom
    glm::vec2 offset{0.0f, 0.0f};       // Pixel offset for rendering

    float getScaleMultiplier() const
    {
        return selected ? 1.1f : 1.0f;
    }
    
    MapSpot() = default;
};
