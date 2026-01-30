#include "Renderer.h"
#include "Window.h"
#include "Scene.h"
#include "systems/CameraSystem.h"
#include "ECS.h"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

Renderer::Renderer(Window* window)
	: m_window(window)
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

	// Swap buffers
	m_window->swapBuffers();
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