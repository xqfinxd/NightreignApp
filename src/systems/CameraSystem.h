#pragma once
#include "public.h"
#include <entt/entt.hpp>
#include "components/Camera.h"

class CameraSystem
{
public:
    CameraSystem();
    ~CameraSystem();

    // Update all cameras in the registry
    void update(entt::registry& registry);
    
    // Get the primary camera (first camera found)
    entt::entity getPrimaryCamera(const entt::registry& registry);
    
    // Create a default camera entity
    static entt::entity createCamera(entt::registry& registry, 
                                     const glm::vec3& position = glm::vec3(0.0f, 0.0f, 5.0f),
                                     const glm::vec3& target = glm::vec3(0.0f, 0.0f, 0.0f));
    
    // Update camera matrices
    static void updateCameraMatrices(Camera& camera);
    
private:

};
