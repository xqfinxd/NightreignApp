#pragma once
#include "public.h"
#include <entt/entt.hpp>

class RenderSystem;
class Device;
class Camera;

class Scene
{
public:
	Scene();
	~Scene();

	void initialize();
	void cleanup();

	void update(float deltaTime);
	void render();
	void drawUI();

	// ECS access
	entt::registry& getRegistry() { return m_registry; }
	const entt::registry& getRegistry() const { return m_registry; }

	Camera* getCamera();
	const Camera* getCamera() const;

	// Get clear color from camera
	glm::vec4 getClearColor() const;

private:
	entt::registry m_registry;
	RenderSystem* m_render_system = nullptr;
};
