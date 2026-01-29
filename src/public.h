#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include <glad/glad.h>
#endif