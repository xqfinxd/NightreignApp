#include "Scene.h"
#include <imgui.h>
#include <SDL_log.h>

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::initialize()
{
	SDL_Log("Scene initialized");
}

void Scene::cleanup()
{
	SDL_Log("Scene cleanup");
}

void Scene::update(float deltaTime)
{
	// Scene update logic
}

void Scene::draw()
{
	// ImGui demo window for testing
	ImGui::Begin("Scene Window");
    ImGui::Text("This is the scene rendering.");
    ImGui::End();
	
	// Add scene-specific rendering here
}
