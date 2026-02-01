#pragma once
#include "public.h"
#include <entt/entt.hpp>

class ResourceManager;
struct MeshComponent;
class Camera;

class RenderSystem
{
public:
    RenderSystem(ResourceManager* resourceManager);
    ~RenderSystem();

    // Render all entities with MeshComponent
    void render(entt::registry& registry);
    
    // Render text labels for spots
    void renderSpotLabels(entt::registry& registry, const Camera& camera);

private:
    void renderEntity(entt::entity entity, const MeshComponent& meshComp, const Camera& camera, entt::registry& registry);
    void setupVertexAttributes();

private:
    ResourceManager* m_resourceManager = nullptr;
};
