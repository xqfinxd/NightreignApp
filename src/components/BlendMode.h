#pragma once
#include "public.h"

// Component to control blend mode for rendering
struct BlendMode
{
    enum class Type {
        Standard,    // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA (for map tiles)
        DestAlpha    // GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA (for spots)
    };
    
    Type mode = Type::Standard;
    
    BlendMode() = default;
    BlendMode(Type m) : mode(m) {}
};
