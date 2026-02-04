#pragma once
#include "public.h"
#include <entt/entt.hpp>

class Device;
class ResourceManager;
struct MeshComponent;
class Camera;

class Renderer
{
public:
	Renderer(Device* device, ResourceManager* resmgr);
	~Renderer();

	void initialize();
	void cleanup();

	void beginFrame();
	void endFrame();

	void clear(const glm::vec4& color);
	void setViewport(int x, int y, int width, int height);

	void drawScene(class Scene* scene);
	
	// Rendering methods (moved from RenderSystem)
	void renderEntities(entt::registry& registry);
	void renderSpotLabels(entt::registry& registry, const Camera& camera, glm::vec4 bgColor, glm::vec4 fgColor);

private:
	void renderEntity(entt::entity entity, const MeshComponent& meshComp, const Camera& camera, entt::registry& registry);
	void setupVertexAttributes();

	Device* m_device = nullptr;
	ResourceManager* m_resource_mgr = nullptr;
};

