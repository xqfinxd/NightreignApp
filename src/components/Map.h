#pragma once
#include "public.h"
#include <string>
#include <glm/glm.hpp>
#include <SDL.h>

// Component for map spots/markers
struct MapSpot
{
    static float textScale;             // Adjust this to change text size
    static float iconSize;              // Base icon size
    int metadata = -1;                  // Spot ID from GameData
    MapPoint point;                     // Position on the map

    MapSpot() = default;
};

struct MapTile {
    int map = -1;
    int x = 0, y = 0;
    int layer = 0;
    bool b1 = false;
    static const char* getAlias(int map, int x, int y, int layer, bool b1) {
        static char s_buf[128] = { '\0' };
        int len = 0;
        if (b1)
            len = SDL_snprintf(s_buf, SDL_arraysize(s_buf), "tile_%d_L%d_%02d_%02d_B1", map, layer, x, y);
        else
            len = SDL_snprintf(s_buf, SDL_arraysize(s_buf), "tile_%d_L%d_%02d_%02d", map, layer, x, y);
        s_buf[len] = '\0';
        return s_buf;
    }
    MapTile(int map_, int x_, int y_, int layer_, bool b1_) : map(map_), x(x_), y(y_), layer(layer_), b1(b1_) {}
};

struct Interactable {
    bool selected = false;
    float scaleMultipler() const { return 1.2f; }
};