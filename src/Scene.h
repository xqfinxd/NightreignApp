#pragma once
#include "public.h"
#include <entt/entt.hpp>

class CameraSystem;
class RenderSystem;
class Device;

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

	// System access
	CameraSystem* getCameraSystem() const { return m_camera_system; }
	
	// Get clear color from camera
	glm::vec4 getClearColor() const;

private:
	entt::registry m_registry;
	CameraSystem* m_camera_system = nullptr;
	RenderSystem* m_render_system = nullptr;
};
