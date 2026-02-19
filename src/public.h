#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define CHS(x) (x)

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

void showToast(const std::string& message, int duration = 1000);
void setInfoPanelContent(const std::string& htmlContent);