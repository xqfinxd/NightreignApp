#include "ResourceManager.h"
#include "Shader.h"
#include "Device.h"
#include "Buffer.h"
#include <SDL_log.h>

ResourceManager* ResourceManager::s_instance = nullptr;

ResourceManager::ResourceManager(Device* device)
    : m_device(device)
{
    s_instance = this;
}

ResourceManager::~ResourceManager()
{
    cleanup();
    s_instance = nullptr;
}

void ResourceManager::initialize()
{
    SDL_Log("ResourceManager: Initialized");
}

void ResourceManager::cleanup()
{
    unloadAll();
    SDL_Log("ResourceManager: Cleanup complete");
}

Shader* ResourceManager::loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath)
{
    // Check if shader already exists
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Shader '%s' already loaded, returning existing shader", name.c_str());
        return it->second;
    }

    // Create and load new shader
    Shader* shader = new Shader();
    if (!shader->loadFromFiles(vertPath, fragPath)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Failed to load shader '%s'", name.c_str());
        delete shader;
        return nullptr;
    }

    m_shaders[name] = shader;
    SDL_Log("ResourceManager: Shader '%s' loaded successfully", name.c_str());
    return shader;
}

Shader* ResourceManager::getShader(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second;
    }
    
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Shader '%s' not found", name.c_str());
    return nullptr;
}

void ResourceManager::unloadShader(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        delete it->second;
        m_shaders.erase(it);
        SDL_Log("ResourceManager: Shader '%s' unloaded", name.c_str());
    }
}

uint32_t ResourceManager::loadTexture(const std::string& name, const std::string& path, bool generateMipmaps)
{
    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: No device available for texture loading");
        return 0;
    }

    // Check if texture already exists
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Texture '%s' already loaded, returning existing texture", name.c_str());
        return it->second;
    }

    // Load texture through device
    uint32_t textureID = m_device->createTexture(path);
    if (textureID == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Failed to load texture '%s' from '%s'", name.c_str(), path.c_str());
        return 0;
    }

    m_textures[name] = textureID;
    SDL_Log("ResourceManager: Texture '%s' loaded successfully (ID: %u)", name.c_str(), textureID);
    return textureID;
}

uint32_t ResourceManager::getTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }
    
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Texture '%s' not found", name.c_str());
    return 0;
}

void ResourceManager::unloadTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        if (m_device) {
            m_device->deleteTexture(it->second);
        }
        m_textures.erase(it);
        SDL_Log("ResourceManager: Texture '%s' unloaded", name.c_str());
    }
}

MeshBuffer* ResourceManager::getMesh(const std::string& name)
{
    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        return it->second;
    }
    
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Mesh '%s' not found", name.c_str());
    return nullptr;
}

void ResourceManager::unloadMesh(const std::string& name)
{
    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        delete it->second;
        m_meshes.erase(it);
        SDL_Log("ResourceManager: Mesh '%s' unloaded", name.c_str());
    }
}

void ResourceManager::unloadAll()
{
    // Unload all shaders
    for (auto& pair : m_shaders) {
        delete pair.second;
    }
    m_shaders.clear();
    SDL_Log("ResourceManager: All shaders unloaded");

    // Unload all textures
    if (m_device) {
        for (auto& pair : m_textures) {
            m_device->deleteTexture(pair.second);
        }
    }
    m_textures.clear();
    SDL_Log("ResourceManager: All textures unloaded");

    // Unload all meshes
    for (auto& pair : m_meshes) {
        delete pair.second;
    }
    m_meshes.clear();
    SDL_Log("ResourceManager: All meshes unloaded");
}
