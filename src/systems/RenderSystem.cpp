#include "RenderSystem.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "Buffer.h"
#include "components/Mesh.h"
#include "components/Transform.h"
#include <SDL_log.h>

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

    for (auto entity : viewEntities) {
        auto& meshComp = viewEntities.get<MeshComponent>(entity);

        // Skip if not visible
        if (!meshComp.visible) {
            continue;
        }

        // Get resources by name
        MeshBuffer* mesh = m_resourceManager->getMesh(meshComp.meshName);
        Shader* shader = m_resourceManager->getShader(meshComp.shaderName);

        if (!mesh || !shader) {
            continue;
        }

        // Get buffers
        Buffer* vertexBuffer = mesh->getVertexBuffer();
        Buffer* indexBuffer = mesh->getIndexBuffer();

        if (!vertexBuffer || !vertexBuffer->isValid()) {
            continue;
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

        // Bind texture if specified
        if (!meshComp.textureName.empty()) {
            uint32_t textureId = m_resourceManager->getTexture(meshComp.textureName);
            if (textureId != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureId);
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
}

void RenderSystem::setupVertexAttributes()
{
    // Setup for Vertex structure:
    // struct Vertex {
    //     glm::vec3 position;  // offset 0
    //     glm::vec3 normal;    // offset 12
    //     glm::vec2 texCoord;  // offset 24
    //     glm::vec4 color;     // offset 32
    // };
    size_t stride = sizeof(float) * (3 + 3 + 2 + 4); // 48 bytes

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 3));

    // TexCoord attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 6));

    // Color attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * 8));
}
