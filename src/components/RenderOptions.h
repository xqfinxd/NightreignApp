#pragma once
#include "public.h"

enum class BlendType {
    Standard,    // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA (for map tiles)
    DestAlpha    // GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA (for spots)
};

// Component to control blend mode for rendering
struct RenderOptions
{
    BlendType mode = BlendType::Standard;
    float order = 0.0f; // Optional: for controlling render order if needed
    
    RenderOptions() = default;
    RenderOptions(BlendType m) : mode(m) {}
    RenderOptions(BlendType m, float o) : mode(m), order(o) {}
};
