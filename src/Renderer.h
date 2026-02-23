#pragma once
#include "public.h"
#include "components/RenderOptions.h"
#include <entt/entt.hpp>

class Device;
class ResourceManager;
struct MeshComponent;
class Camera;
class Entity;

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
	void renderEntities(const Camera& camera, const std::vector<Entity*>& gameObjects);
	void renderText(const Camera& camera, const std::vector<Entity*>& gameObjects, glm::vec4 bgColor, glm::vec4 fgColor);

private:
	void renderEntity(const Camera& camera, Entity& gameObject);
	void setupVertexAttributes();
	void applyBlendFunc(BlendType type);

	Device* m_device = nullptr;
	ResourceManager* m_resource_mgr = nullptr;
};

