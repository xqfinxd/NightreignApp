#pragma once
#include "public.h"
#include <string>
#include <vector>

// Component for a grid of map tiles
struct MapTileGrid
{
    int mapIndex = 0;           // Which texture folder (0-5)
    int layer = 0;              // Layer (L0, L1, etc.)
    int gridWidth = 6;          // Number of tiles horizontally
    int gridHeight = 6;         // Number of tiles vertically
    float tileSize = 1.0f;      // Size of each tile in world units
    
    std::vector<std::string> tileTextures; // Texture names for each tile
    
    MapTileGrid() = default;
    
    MapTileGrid(int index, int l, int w, int h, float size = 1.0f)
        : mapIndex(index)
        , layer(l)
        , gridWidth(w)
        , gridHeight(h)
        , tileSize(size)
    {}
};
