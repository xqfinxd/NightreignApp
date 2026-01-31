#include "Renderer.h"
#include "Device.h"
#include "ResourceManager.h"
#include "Scene.h"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

Renderer::Renderer(Device* device, ResourceManager* resmgr)
	: m_device(device), m_resource_mgr(resmgr)
{
}

Renderer::~Renderer()
{
}

void Renderer::initialize()
{
	SDL_Log("Renderer initialized");
}

void Renderer::cleanup()
{
	SDL_Log("Renderer cleanup");
}

void Renderer::beginFrame()
{
	// Start ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void Renderer::endFrame()
{
	// Render ImGui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::clear(const glm::vec4& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setViewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

void Renderer::drawScene(Scene* scene)
{
	if (scene) {
		// Clear with scene's camera clear color
		clear(scene->getClearColor());
		
		// Render scene content
		scene->render();
	}
}