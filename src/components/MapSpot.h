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
    int spotId = -1;                    // Spot ID from GameData
    std::string label;                  // Text label to display below spot
    bool visible = true;                // Is this spot visible
    bool selected = false;              // Is this spot selected

    float getScaleMultiplier() const
    {
        return selected ? 1.1f : 1.0f;
    }
    
    MapSpot() = default;
};
