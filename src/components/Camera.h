#pragma once
#include "public.h"

struct Camera {
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    float fov = 45.0f;
    float aspect = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    bool isOrthographic = false;
    float orthoSize = 10.0f;
    
    glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.15f, 1.0f);

    // Getters for matrices
    glm::mat4 getViewMatrix() const {
        return view;
    }

    glm::mat4 getProjectionMatrix() const {
        return projection;
    }
};
