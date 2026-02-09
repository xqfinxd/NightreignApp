#pragma once
#include "public.h"

class Camera
{
public:
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float fov = 45.0f;
    float aspect = 16.0f / 9.0f;
    float nearPlane = 0.01f;
    float farPlane = 1000.0f;

    bool isOrthographic = true;
    float orthoSize = 5.0f;

    glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.15f, 1.0f);

    void setAspect(float width, float height)
    {
        float newAspect = width / height;
        if (std::abs(aspect - newAspect) > FLT_EPSILON)
        {
            aspect = newAspect;
            updateMatrices();
        }
    }

    // Getters for matrices
    glm::mat4 getViewMatrix() const
    {
        return view;
    }

    glm::mat4 getProjectionMatrix() const
    {
        return projection;
    }

    void updateMatrices()
    {
        // Update view matrix
        view = glm::lookAt(position, target, up);

        // Update projection matrix
        if (isOrthographic)
        {
            float halfHeight = orthoSize * 0.5f;
            float halfWidth = halfHeight * aspect;
            projection = glm::ortho(-halfWidth, halfWidth,
                                           -halfHeight, halfHeight,
                                           nearPlane, farPlane);
        }
        else
        {
            projection = glm::perspective(glm::radians(fov),
                                                 aspect,
                                                 nearPlane,
                                                 farPlane);
        }
    }

    glm::vec4 clip2World(glm::vec4 clipCoords) const
    {
        glm::mat4 inverseVP = glm::inverse(projection * view);
        return inverseVP * clipCoords;
    }
};
