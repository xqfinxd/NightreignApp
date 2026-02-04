#pragma once
#include "public.h"
#include "Buffer.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <SDL_log.h>

class Shader;
class Device;
class IResource;
class Texture;

class ResourceManager
{
public:
    ResourceManager(Device* device);
    ~ResourceManager();

    void initialize();
    void cleanup();

    // Shader management
    Shader* loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    Shader* getShader(const std::string& name);
    void unloadShader(const std::string& name);

    // Texture management
    Texture* loadTexture(const std::string& name, const std::string& path, bool generateMipmaps = false);
    Texture* getTexture(const std::string& name);
    void unloadTexture(const std::string& name);

    // Mesh buffer management
    template<typename VertexType>
    MeshBuffer* createMesh(const std::string& name, const std::vector<VertexType>& vertices);
    
    template<typename VertexType, typename IndexType>
    MeshBuffer* createMesh(const std::string& name, const std::vector<VertexType>& vertices, const std::vector<IndexType>& indices);
    
    MeshBuffer* getMesh(const std::string& name);
    void unloadMesh(const std::string& name);

    // Utility
    void unloadAll();
    size_t getShaderCount() const { return m_shaders.size(); }
    size_t getTextureCount() const { return m_textures.size(); }
    size_t getMeshCount() const { return m_meshes.size(); }

    // Singleton access (optional)
    static ResourceManager* getInstance() { return s_instance; }

private:
    std::unordered_map<std::string, Shader*> m_shaders;
    std::unordered_map<std::string, Texture*> m_textures;
    std::unordered_map<std::string, MeshBuffer*> m_meshes;
    
    Device* m_device = nullptr;
    static ResourceManager* s_instance;
};

// Template implementations
template<typename VertexType>
MeshBuffer* ResourceManager::createMesh(const std::string& name, const std::vector<VertexType>& vertices)
{
    // Check if mesh already exists
    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Mesh '%s' already exists", name.c_str());
        return it->second;
    }

    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: No device available");
        return nullptr;
    }

    MeshBuffer* mesh = new MeshBuffer(m_device);
    mesh->setVertexData(vertices);
    m_meshes[name] = mesh;
    
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Mesh '%s' created (%zu vertices)", name.c_str(), vertices.size());
    return mesh;
}

template<typename VertexType, typename IndexType>
MeshBuffer* ResourceManager::createMesh(const std::string& name, const std::vector<VertexType>& vertices, const std::vector<IndexType>& indices)
{
    // Check if mesh already exists
    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Mesh '%s' already exists", name.c_str());
        return it->second;
    }

    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: No device available");
        return nullptr;
    }

    MeshBuffer* mesh = new MeshBuffer(m_device);
    mesh->setVertexData(vertices);
    mesh->setIndexData(indices);
    m_meshes[name] = mesh;
    
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Mesh '%s' created (%zu vertices, %zu indices)", 
            name.c_str(), vertices.size(), indices.size());
    return mesh;
}
