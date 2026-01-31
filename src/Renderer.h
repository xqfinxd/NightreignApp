#pragma once
#include "public.h"

class Device;
class ResourceManager;

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

private:
	Device* m_device = nullptr;
	ResourceManager* m_resource_mgr = nullptr;
};

