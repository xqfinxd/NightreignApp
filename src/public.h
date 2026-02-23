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

#ifdef _DEBUG
#define PROPERTY 
#else
#define PROPERTY static constexpr
#endif

struct MapPoint
{
	int gridXNo = 0, gridZNo = 0;
	float posX = 0.0f, posZ = 0.0f;
    float height = 0.f;

    int getX() const {
        return gridXNo - 41;
    }
    int getZ() const {
        return gridZNo - 35;
    }

    glm::vec2 normalize(int tileSize = 256) const {
        float gridX = posX / tileSize + getX();
        float gridZ = posZ / tileSize + getZ();
        return glm::vec2(gridX, gridZ);
	}
};

void showToast(const std::string& message, int duration = 1000);
void setInfoPanelContent(const std::string& htmlContent);