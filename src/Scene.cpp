#include "Scene.h"
#include "systems/CameraSystem.h"
#include "systems/RenderSystem.h"
#include "ResourceManager.h"
#include "components/Mesh.h"
#include "components/Transform.h"
#include "Device.h"
#include "ECS.h"
#include <imgui.h>
#include <SDL_log.h>
#include <entt/entt.hpp>

Scene::Scene()
{
	m_camera_system = new CameraSystem();
	SDL_Log("Scene: ECS registry created");
}

Scene::~Scene()
{
	delete m_render_system;
	delete m_camera_system;
	SDL_Log("Scene: ECS registry destroyed (entities: %zu)", m_registry.size());
}

void Scene::initialize()
{
	// Create default camera
	CameraSystem::createCamera(m_registry, 
	                          glm::vec3(0.0f, 0.0f, 5.0f),
	                          glm::vec3(0.0f, 0.0f, 0.0f));
	
	// Get ResourceManager
	ResourceManager* resMgr = ResourceManager::getInstance();
	if (!resMgr) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Scene: ResourceManager not available");
		return;
	}

	// Create RenderSystem
	m_render_system = new RenderSystem(resMgr);

	// Load shader and texture
	resMgr->loadShader("texture", "assets/shaders/texture.vert", "assets/shaders/texture.frag");
	resMgr->loadTexture("bg", "assets/textures/bg.png");

	// Create a simple quad mesh for background
	std::vector<Vertex> quadVertices = {
		Vertex(glm::vec3(-2.0f, -2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)),
		Vertex(glm::vec3( 2.0f, -2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)),
		Vertex(glm::vec3( 2.0f,  2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
		Vertex(glm::vec3(-2.0f,  2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f))
	};

	std::vector<uint32_t> quadIndices = {
		0, 1, 2,
		2, 3, 0
	};

	resMgr->createMesh("quad", quadVertices, quadIndices);

	// Create background entity
	auto bgEntity = m_registry.create();
	m_registry.emplace<MeshComponent>(bgEntity, "quad", "texture", "bg");
	m_registry.emplace<Transform>(bgEntity);
	
	SDL_Log("Scene initialized (ECS ready, entities: %zu)", m_registry.size());
}

void Scene::cleanup()
{
	SDL_Log("Scene cleanup (clearing %zu entities)", m_registry.size());
	m_registry.clear();
}

void Scene::update(float deltaTime)
{
	// Update camera system
	m_camera_system->update(m_registry);
	
	// Scene update logic
}

void Scene::render()
{
	if (!m_render_system) {
		return;
	}

	// Get camera matrices
	auto cameraEntity = m_camera_system->getPrimaryCamera(m_registry);
	if (cameraEntity == entt::null) {
		return;
	}

	auto& camera = m_registry.get<Camera>(cameraEntity);
	glm::mat4 view = camera.getViewMatrix();
	glm::mat4 projection = camera.getProjectionMatrix();

	// Render all entities with MeshComponent
	m_render_system->render(m_registry);
}

void Scene::drawUI()
{
    ImGui::Text("ECS Statistics:");
    ImGui::Text("  Entities: %zu", m_registry.size());
    ImGui::Text("  Alive: %zu", m_registry.alive());
    
    ImGui::Separator();
    ImGui::Text("Camera:");
    auto cameraEntity = m_camera_system->getPrimaryCamera(m_registry);
    if (cameraEntity != entt::null) {
        auto& camera = m_registry.get<Camera>(cameraEntity);
        ImGui::Text("  Position: (%.2f, %.2f, %.2f)", 
                    camera.position.x, camera.position.y, camera.position.z);
        ImGui::Text("  FOV: %.1f°", camera.fov);
        ImGui::Text("  Type: %s", camera.isOrthographic ? "Orthographic" : "Perspective");
    } else {
        ImGui::Text("  No camera");
    }
}

glm::vec4 Scene::getClearColor() const
{
	auto cameraEntity = m_camera_system->getPrimaryCamera(m_registry);
	if (cameraEntity != entt::null) {
		auto& camera = m_registry.get<Camera>(cameraEntity);
		return camera.clearColor;
	}
	return glm::vec4(0.1f, 0.1f, 0.15f, 1.0f); // Default color
}
