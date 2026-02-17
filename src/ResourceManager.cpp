#include "ResourceManager.h"
#include "Shader.h"
#include "Device.h"
#include "Buffer.h"
#include "Texture.h"
#include "AsyncResourceLoader.h"
#include <SDL_log.h>

ResourceManager* ResourceManager::s_instance = nullptr;

ResourceManager::ResourceManager(Device* device)
    : m_device(device)
{
    s_instance = this;
    m_async_loader = new AsyncResourceLoader(this);
}

ResourceManager::~ResourceManager()
{
    cleanup();
    delete m_async_loader;
    s_instance = nullptr;
}

void ResourceManager::initialize()
{
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Initialized");
}

void ResourceManager::cleanup()
{
    unloadAll();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Cleanup complete");
}

Shader* ResourceManager::loadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath)
{
    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: No device available for shader loading");
        return nullptr;
    }

    // Check if shader already exists
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Shader '%s' already loaded, returning existing shader", name.c_str());
        return it->second;
    }

    // Create shader through device
    Shader* shader = m_device->createShader(vertPath, fragPath);
    if (!shader || !shader->isValid()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Failed to load shader '%s'", name.c_str());
        return nullptr;
    }

    m_shaders[name] = shader;
    SDL_Log("ResourceManager: Shader '%s' loaded successfully (Program ID: %u)", name.c_str(), shader->getProgram());
    return shader;
}

Shader* ResourceManager::getShader(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        return it->second;
    }
    
    return nullptr;
}

void ResourceManager::unloadShader(const std::string& name)
{
    auto it = m_shaders.find(name);
    if (it != m_shaders.end()) {
        if (m_device) {
            m_device->deleteShader(it->second);
        }
        m_shaders.erase(it);
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Shader '%s' unloaded", name.c_str());
    }
}

Texture* ResourceManager::loadTexture(const std::string& name, const std::string& path, bool generateMipmaps)
{
    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: No device available for texture loading");
        return nullptr;
    }

    // Check if texture already exists
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }

    // Load texture through device
    Texture* texture = m_device->createTexture(path);
    if (!texture || !texture->isValid()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Failed to load texture '%s' from '%s'", name.c_str(), path.c_str());
        return nullptr;
    }

    m_textures[name] = texture;
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Texture '%s' loaded successfully (ID: %u, Size: %dx%d)", 
        name.c_str(), texture->getId(), texture->getWidth(), texture->getHeight());
    return texture;
}

Texture* ResourceManager::getTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }
    
    return nullptr;
}

void ResourceManager::unloadTexture(const std::string& name)
{
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        if (m_device) {
            m_device->deleteTexture(it->second);
        }
        m_textures.erase(it);
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Texture '%s' unloaded", name.c_str());
    }
}

MeshBuffer* ResourceManager::getMesh(const std::string& name)
{
    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        return it->second;
    }
    
    return nullptr;
}

void ResourceManager::unloadMesh(const std::string& name)
{
    auto it = m_meshes.find(name);
    if (it != m_meshes.end()) {
        delete it->second;
        m_meshes.erase(it);
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Mesh '%s' unloaded", name.c_str());
    }
}

void ResourceManager::unloadAll()
{
    // Unload all shaders
    if (m_device) {
        for (auto& pair : m_shaders) {
            m_device->deleteShader(pair.second);
        }
    }
    m_shaders.clear();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: All shaders unloaded");

    // Unload all textures
    if (m_device) {
        for (auto& pair : m_textures) {
            m_device->deleteTexture(pair.second);
        }
    }
    m_textures.clear();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: All textures unloaded");

    // Unload all meshes
    for (auto& pair : m_meshes) {
        delete pair.second;
    }
    m_meshes.clear();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: All meshes unloaded");
}
