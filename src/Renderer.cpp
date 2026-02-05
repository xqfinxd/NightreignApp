#include "Renderer.h"
#include "Device.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "components/Camera.h"
#include "Shader.h"
#include "Buffer.h"
#include "Texture.h"
#include "components/Mesh.h"
#include "components/Transform.h"
#include "components/RenderOptions.h"
#include "components/MapSpot.h"
#include <SDL.h>
#include <SDL_log.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <algorithm>
#include <vector>

Renderer::Renderer(Device* device, ResourceManager* resmgr)
	: m_device(device), m_resource_mgr(resmgr)
{
}

Renderer::~Renderer()
{
}

void Renderer::initialize()
{
	// Load shaders
	m_resource_mgr->loadShader("texture", "nightreign/assets/shaders/texture.vert", "nightreign/assets/shaders/texture.frag");
	m_resource_mgr->loadShader("font", "nightreign/assets/shaders/font.vert", "nightreign/assets/shaders/font.frag");

	// Create a simple quad mesh for tiles
	std::vector<Vertex> quadVertices = {
		Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f)),
		Vertex(glm::vec3(0.5f, -0.5f, 0.0f) , glm::vec2(1.0f, 0.0f), glm::vec4(1.0f)),
		Vertex(glm::vec3(0.5f, 0.5f, 0.0f)  , glm::vec2(1.0f, 1.0f), glm::vec4(1.0f)),
		Vertex(glm::vec3(-0.5f, 0.5f, 0.0f) , glm::vec2(0.0f, 1.0f), glm::vec4(1.0f))};

	std::vector<uint32_t> quadIndices = {
		0, 1, 2,
		2, 3, 0};

	m_resource_mgr->createMesh("quad", quadVertices, quadIndices);

	// Load spot texture
	m_resource_mgr->loadTexture("launch", "nightreign/assets/textures/spots/launch.png");

	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Renderer initialized");
}

void Renderer::cleanup()
{
	SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Renderer cleanup");
}

void Renderer::beginFrame()
{
	// Start ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	glEnable(GL_BLEND);
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
	auto canvasSize = m_device->getCanvasSize();
	if (scene) {
		if(auto camera = scene->getCamera())
		{
			clear(camera->clearColor);
			camera->setAspect(canvasSize.x, canvasSize.y);
			scene->render(this);
		}
	}
}

void Renderer::renderEntities(entt::registry& registry)
{
	if (!m_resource_mgr) {
		return;
	}

	auto cameraEntities = registry.view<Camera>();
	if (cameraEntities.empty()) {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Renderer: No camera found for rendering");
		return;
	}
	// Use the first camera found
	auto& camera = registry.get<Camera>(*cameraEntities.begin());

	// Get all entities with MeshComponent and Transform
	auto viewEntities = registry.view<MeshComponent, Transform>();

	// Collect entities and sort by render order
	std::vector<entt::entity> sortedEntities;
	for (auto entity : viewEntities) {
		auto& meshComp = registry.get<MeshComponent>(entity);
		if (meshComp.visible) {
			sortedEntities.push_back(entity);
		}
	}

	// Sort by RenderOptions.order (default 0 if not present)
	std::sort(sortedEntities.begin(), sortedEntities.end(), 
		[&registry](entt::entity a, entt::entity b) {
			float orderA = 0.0f;
			float orderB = 0.0f;
			
			if (auto optA = registry.try_get<RenderOptions>(a)) {
				orderA = optA->order;
			}
			if (auto optB = registry.try_get<RenderOptions>(b)) {
				orderB = optB->order;
			}
			
			return orderA < orderB;
		});

	// Render entities in sorted order
	for (auto entity : sortedEntities) {
		renderEntity(registry, camera, entity);
	}
}

void Renderer::renderEntity(entt::registry& registry, const Camera& camera, entt::entity entity)
{
	auto& meshComp = registry.get<MeshComponent>(entity);
	auto& transform = registry.get<Transform>(entity);
	// Get resources by name
	MeshBuffer* mesh = m_resource_mgr->getMesh(meshComp.meshName);
	Shader* shader = m_resource_mgr->getShader(meshComp.shaderName);

	if (!mesh || !shader) {
		return;
	}

	// Get buffers
	Buffer* vertexBuffer = mesh->getVertexBuffer();
	Buffer* indexBuffer = mesh->getIndexBuffer();

	if (!vertexBuffer || !vertexBuffer->isValid()) {
		return;
	}

	// Use shader
	shader->use();

	// Set view and projection matrices
	glm::mat4 mvp(1.0f);
	mvp = camera.getProjectionMatrix() * camera.getViewMatrix();

	// Set model matrix if entity has Transform
	shader->setMat4("mvp", mvp * transform.getModelMatrix());

	// Set blend mode based on RenderOptions component
	if (auto* opt = registry.try_get<RenderOptions>(entity)) {
		applyBlendFunc(opt->mode);
	}

	// Bind texture if specified
	if (!meshComp.textureName.empty()) {
		Texture* texture = m_resource_mgr->getTexture(meshComp.textureName);
		if (texture && texture->isValid()) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->getId());
			shader->setInt("mapTexture", 0);
		}
	}

	// Bind vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer->getID());

	// Setup vertex attributes (assuming Vertex structure)
	setupVertexAttributes();

	// Draw
	if (mesh->hasIndices() && indexBuffer) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->getID());
		glDrawElements(GL_TRIANGLES, mesh->getIndexCount(), GL_UNSIGNED_INT, 0);
	} else {
		glDrawArrays(GL_TRIANGLES, 0, mesh->getVertexCount());
	}

	// Cleanup
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::setupVertexAttributes()
{
	// Setup for Vertex structure:
	size_t stride = sizeof(float) * (3 + 2 + 4);

	// Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

	// TexCoord attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));

	// Color attribute
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 5));
}

void Renderer::applyBlendFunc(BlendType type)
{
	switch (type) {
	case BlendType::Standard:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BlendType::DestAlpha:
		glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	default:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}
}

void Renderer::renderSpotLabels(entt::registry& registry, const Camera& camera,
	glm::vec4 bgColor, glm::vec4 fgColor)
{
	// Get ImGui font atlas
	ImGuiIO& io = ImGui::GetIO();
	ImFontAtlas* atlas = io.Fonts;
	if (!atlas || !atlas->TexID)
		return;

	ImFont* font = atlas->Fonts[0];
	if (!font || !font->IsLoaded())
		return;
	
	// Get font shader
	Shader* shader = m_resource_mgr->getShader("font");
	if (!shader)
		return;
	
	shader->use();
	float textScale = MapSpot::textScale;
	
	// Set custom foreground and background colors
	glm::vec4 foregroundColor = fgColor;
	glm::vec4 backgroundColor = bgColor;
	shader->setVec4("foregroundColor", foregroundColor);
	shader->setVec4("backgroundColor", backgroundColor);
	
	// Set up OpenGL state for text rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)atlas->TexID);
	shader->setInt("fontTexture", 0);
	
	// Get view-projection matrix
	glm::mat4 viewProj = camera.getProjectionMatrix() * camera.getViewMatrix();

	// Render labels for all spots
	auto spotView = registry.view<MapSpot, Transform>();
	for (auto entity : spotView)
	{
		auto& spot = spotView.get<MapSpot>(entity);
		auto& transform = spotView.get<Transform>(entity);
		
		if (spot.label.empty())
			continue;
		
		// Calculate text size
		ImVec2 textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, spot.label.c_str());

		// Calculate centered offset
		float textWidth = textSize.x * textScale;
		float textHeight = textSize.y * textScale;
		
		// World-space position for text (below the spot)
		glm::vec3 textWorldPos = transform.position;
		textWorldPos.y -= transform.scale.y * 0.5f + textHeight * 1.2f; // Position below spot
		
		// Draw background rectangle first (if background color has alpha > 0)
		if (backgroundColor.a > 0.0f)
		{
			// Add padding around text
			float padding = textHeight * 0.2f;
			float bgX0 = textWorldPos.x - textWidth * 0.5f - padding;
			float bgX1 = textWorldPos.x + textWidth * 0.5f + padding;
			float bgY0 = textWorldPos.y - textHeight - padding;
			float bgY1 = textWorldPos.y + padding;
			
			// Create background quad with no texture (use dummy UV coords)
			std::vector<Vertex> bgVertices;
			bgVertices.emplace_back(glm::vec3(bgX0, bgY0, textWorldPos.z - 0.001f), glm::vec2(0.0f, 0.0f), backgroundColor);
			bgVertices.emplace_back(glm::vec3(bgX1, bgY0, textWorldPos.z - 0.001f), glm::vec2(0.0f, 0.0f), backgroundColor);
			bgVertices.emplace_back(glm::vec3(bgX1, bgY1, textWorldPos.z - 0.001f), glm::vec2(0.0f, 0.0f), backgroundColor);
			bgVertices.emplace_back(glm::vec3(bgX0, bgY1, textWorldPos.z - 0.001f), glm::vec2(0.0f, 0.0f), backgroundColor);
			
			std::vector<uint32_t> bgIndices = {0, 1, 2, 2, 3, 0};
			
			GLuint bgVbo, bgEbo;
			glGenBuffers(1, &bgVbo);
			glGenBuffers(1, &bgEbo);
			
			glBindBuffer(GL_ARRAY_BUFFER, bgVbo);
			glBufferData(GL_ARRAY_BUFFER, bgVertices.size() * sizeof(bgVertices[0]), bgVertices.data(), GL_DYNAMIC_DRAW);
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bgEbo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, bgIndices.size() * sizeof(uint32_t), bgIndices.data(), GL_DYNAMIC_DRAW);
			
			shader->setMat4("mvp", viewProj);
			shader->setVec4("foregroundColor", backgroundColor);
			shader->setVec4("backgroundColor", backgroundColor);
			
			setupVertexAttributes();
			glDrawElements(GL_TRIANGLES, bgIndices.size(), GL_UNSIGNED_INT, 0);
			
			glDeleteBuffers(1, &bgVbo);
			glDeleteBuffers(1, &bgEbo);
		}
		
		// Render each character
		float cursorX = textWorldPos.x - textWidth * 0.5f; // Center horizontally
		float cursorY = textWorldPos.y;
		
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		uint32_t indexOffset = 0;
		
		// Reset shader colors for text rendering
		shader->setVec4("foregroundColor", foregroundColor);
		shader->setVec4("backgroundColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		
		std::string showLabel = spot.label;
		if(showLabel.empty())
			showLabel = "UNKNOWN";

		// Properly decode UTF-8 characters
		const char* text_begin = showLabel.c_str();
		const char* text_end = text_begin + showLabel.length();
		
		for (const char* s = text_begin; s < text_end; )
		{
			unsigned int c = 0;
			int bytes_count = ImTextCharFromUtf8(&c, s, text_end);
			if (bytes_count <= 0)
				break;
			s += bytes_count;
			
			if (c < 32)
				continue;
			
			const ImFontGlyph* glyph = font->FindGlyph((ImWchar)c);
			if (!glyph)
				continue;
			
			// Calculate glyph quad
			float x0 = cursorX + glyph->X0 * textScale;
			float y0 = cursorY - glyph->Y0 * textScale;
			float x1 = cursorX + glyph->X1 * textScale;
			float y1 = cursorY - glyph->Y1 * textScale;
			
			// UV coordinates from font atlas
			float u0 = glyph->U0;
			float v0 = glyph->V0;
			float u1 = glyph->U1;
			float v1 = glyph->V1;
			
			// Add vertices (position + texcoord + dummy color)
			glm::vec4 whiteColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			// Bottom-left
			vertices.emplace_back(glm::vec3(x0, y0, textWorldPos.z), glm::vec2(u0, v0), whiteColor);
			// Bottom-right
			vertices.emplace_back(glm::vec3(x1, y0, textWorldPos.z), glm::vec2(u1, v0), whiteColor);
			// Top-right
			vertices.emplace_back(glm::vec3(x1, y1, textWorldPos.z), glm::vec2(u1, v1), whiteColor);
			// Top-left
			vertices.emplace_back(glm::vec3(x0, y1, textWorldPos.z), glm::vec2(u0, v1), whiteColor);
			
			// Add indices for two triangles
			indices.push_back(indexOffset + 0);
			indices.push_back(indexOffset + 1);
			indices.push_back(indexOffset + 2);
			indices.push_back(indexOffset + 2);
			indices.push_back(indexOffset + 3);
			indices.push_back(indexOffset + 0);
			
			indexOffset += 4;
			
			// Advance cursor
			cursorX += glyph->AdvanceX * textScale;
		}
		
		if (vertices.empty())
			continue;
		
		// Create temporary buffers and render
		GLuint vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_DYNAMIC_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_DYNAMIC_DRAW);
		
		// Set MVP matrix
		shader->setMat4("mvp", viewProj);
		
		// Setup vertex attributes
		setupVertexAttributes();
		
		// Draw
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		
		// Cleanup
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}