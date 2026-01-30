#include "CameraSystem.h"
#include <SDL_log.h>
#include <glm/gtc/matrix_transform.hpp>

CameraSystem::CameraSystem()
{
    SDL_Log("CameraSystem: Created");
}

CameraSystem::~CameraSystem()
{
    SDL_Log("CameraSystem: Destroyed");
}

void CameraSystem::update(entt::registry& registry)
{
    // Update all camera matrices
    auto view = registry.view<Camera>();
    for (auto entity : view) {
        auto& camera = view.get<Camera>(entity);
        updateCameraMatrices(camera);
    }
}

entt::entity CameraSystem::getPrimaryCamera(const entt::registry& registry)
{
    auto view = registry.view<Camera>();
    for (auto entity : view) {
        return entity; // Return first camera found
    }
    return entt::null;
}

entt::entity CameraSystem::createCamera(entt::registry& registry, 
                                        const glm::vec3& position,
                                        const glm::vec3& target)
{
    auto entity = registry.create();
    auto& camera = registry.emplace<Camera>(entity);
    
    camera.position = position;
    camera.target = target;
    
    updateCameraMatrices(camera);
    
    SDL_Log("CameraSystem: Camera created at (%.2f, %.2f, %.2f)", 
            position.x, position.y, position.z);
    
    return entity;
}

void CameraSystem::updateCameraMatrices(Camera& camera)
{
    // Update view matrix
    camera.view = glm::lookAt(camera.position, camera.target, camera.up);
    
    // Update projection matrix
    if (camera.isOrthographic) {
        float halfHeight = camera.orthoSize * 0.5f;
        float halfWidth = halfHeight * camera.aspect;
        camera.projection = glm::ortho(-halfWidth, halfWidth, 
                                      -halfHeight, halfHeight,
                                      camera.nearPlane, camera.farPlane);
    } else {
        camera.projection = glm::perspective(glm::radians(camera.fov), 
                                            camera.aspect,
                                            camera.nearPlane, 
                                            camera.farPlane);
    }
}
