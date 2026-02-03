#include "RenderSystem.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "Buffer.h"
#include "Texture.h"
#include "components/Mesh.h"
#include "components/Transform.h"
#include "components/Camera.h"
#include "components/BlendMode.h"
#include "components/MapSpot.h"
#include <SDL_log.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>

RenderSystem::RenderSystem(ResourceManager* resourceManager)
    : m_resourceManager(resourceManager)
{
    SDL_Log("RenderSystem: Created");
}

RenderSystem::~RenderSystem()
{
    SDL_Log("RenderSystem: Destroyed");
}

void RenderSystem::render(entt::registry& registry)
{
    if (!m_resourceManager) {
        return;
    }

    auto cameraEntities = registry.view<Camera>();
    if (cameraEntities.empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "RenderSystem: No camera found for rendering");
        return;
    }
    // Use the first camera found
    auto& camera = registry.get<Camera>(*cameraEntities.begin());

    // Get all entities with MeshComponent
    auto viewEntities = registry.view<MeshComponent>();

    // First pass: render map tiles (entities without DestAlpha blend mode)
    for (auto entity : viewEntities) {
        auto& meshComp = viewEntities.get<MeshComponent>(entity);

        // Skip if not visible
        if (!meshComp.visible) {
            continue;
        }

        // Skip spots in first pass
        if (registry.all_of<BlendMode>(entity)) {
            auto& blendMode = registry.get<BlendMode>(entity);
            if (blendMode.mode == BlendMode::Type::DestAlpha) {
                continue;
            }
        }

        renderEntity(entity, meshComp, camera, registry);
    }

    // Second pass: render spots (entities with DestAlpha blend mode)
    for (auto entity : viewEntities) {
        auto& meshComp = viewEntities.get<MeshComponent>(entity);

        // Skip if not visible
        if (!meshComp.visible) {
            continue;
        }

        // Only render spots in second pass
        bool isSpot = false;
        if (registry.all_of<BlendMode>(entity)) {
            auto& blendMode = registry.get<BlendMode>(entity);
            isSpot = (blendMode.mode == BlendMode::Type::DestAlpha);
        }
        if (!isSpot) {
            continue;
        }

        renderEntity(entity, meshComp, camera, registry);
    }
}

void RenderSystem::renderEntity(entt::entity entity, const MeshComponent& meshComp, const Camera& camera, entt::registry& registry)
{
    // Get resources by name
    MeshBuffer* mesh = m_resourceManager->getMesh(meshComp.meshName);
    Shader* shader = m_resourceManager->getShader(meshComp.shaderName);

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
    if (registry.all_of<Transform>(entity)) {
        auto& transform = registry.get<Transform>(entity);
        shader->setMat4("mvp", mvp * transform.getModelMatrix());
    }

    // Set blend mode based on BlendMode component
    if (registry.all_of<BlendMode>(entity)) {
        auto& blendMode = registry.get<BlendMode>(entity);
        if (blendMode.mode == BlendMode::Type::DestAlpha) {
            glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    } else {
        // Default blend mode for entities without BlendMode component
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Bind texture if specified
    if (!meshComp.textureName.empty()) {
        Texture* texture = m_resourceManager->getTexture(meshComp.textureName);
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

void RenderSystem::setupVertexAttributes()
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
void RenderSystem::renderSpotLabels(entt::registry& registry, const Camera& camera,
    glm::vec4 bgColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4 fgColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f))
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
    Shader* shader = m_resourceManager->getShader("font");
    if (!shader)
        return;
    
    shader->use();
    float textScale = MapSpot::textScale;
    
    // Set custom foreground and background colors
    glm::vec4 foregroundColor = fgColor; // Use passed foreground color
    glm::vec4 backgroundColor = bgColor; // Use passed background color
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

        // Scale factor for text in world space

        // Calculate centered offset
        float textWidth = textSize.x * textScale;
        float textHeight = textSize.y * textScale;
        
        // World-space position for text (below the spot)
        glm::vec3 textWorldPos = transform.position;
        textWorldPos.y -= spot.size * 0.5f + textHeight * 1.2f; // Position below spot
        
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