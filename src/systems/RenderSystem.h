#pragma once
#include "public.h"
#include <entt/entt.hpp>

class ResourceManager;

class RenderSystem
{
public:
    RenderSystem(ResourceManager* resourceManager);
    ~RenderSystem();

    // Render all entities with MeshComponent
    void render(entt::registry& registry);

private:
    void setupVertexAttributes();

private:
    ResourceManager* m_resourceManager = nullptr;
};
