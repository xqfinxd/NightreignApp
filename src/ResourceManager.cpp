#include "ResourceManager.h"
#include "Shader.h"
#include "Device.h"
#include "Buffer.h"
#include "Texture.h"
#include "TextureRegistry.h"
#include <SDL_log.h>

ResourceManager* ResourceManager::s_instance = nullptr;

ResourceManager::ResourceManager(Device* device)
    : m_device(device)
{
    s_instance = this;
    m_texture_registry = new TextureRegistry();
}

ResourceManager::~ResourceManager()
{
    cleanup();
    delete m_texture_registry;
    s_instance = nullptr;
}

void ResourceManager::initialize()
{
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Initialized");
    
    m_texture_registry->loadAtlasJson("nightreign/atlas.json");
    m_texture_registry->loadCompositeAtlasJson("nightreign/composite_atlas.json");
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
        return it->second;
    }

    // Create shader through device
    Shader* shader = m_device->createShader(vertPath, fragPath);
    if (!shader || !shader->isValid()) {
        return nullptr;
    }

    m_shaders[name] = shader;
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Shader '%s' loaded successfully (Program ID: %u)",
        name.c_str(), shader->getProgram());
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

    const TextureMetadata* metadata = m_texture_registry->GetMetadata(name);
    
    if (!metadata) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Texture '%s' not found in atlas registry", name.c_str());
        return nullptr;
    }

    GLuint placeholderId = m_texture_registry->CreatePlaceholderForAlias(name);
    if (placeholderId == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Failed to create placeholder for texture '%s'", name.c_str());
        return nullptr;
    }

    Texture* placeholderTexture = new Texture(placeholderId, metadata->width, metadata->height, metadata->channel);
    m_textures[name] = placeholderTexture;

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Created placeholder for '%s' (ID: %u, Size: %dx%d)",
        name.c_str(), placeholderId, metadata->width, metadata->height);

    if (!metadata->layers.empty()) {
        for (const auto& layer : metadata->layers) {
            if (!getTexture(layer.sourceAlias))
                loadTexture(layer.sourceAlias);
        }
        bool baked = m_texture_registry->BakeComposite(name);
        GLuint id = m_texture_registry->GetTextureId(name);
        if (!baked || id == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                "ResourceManager: Failed to bake composite texture '%s'", name.c_str());
            return placeholderTexture;
        }
    } else if (!metadata->path.empty()) {
        m_texture_registry->loadTextureDataAsync(name,
            [name](bool success) {
                if (success) {
                    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Async texture loaded: %s", name.c_str());
                }
                else {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ResourceManager: Failed to load async texture: %s", name.c_str());
                }
            }
        );
    }

    return placeholderTexture;
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

bool ResourceManager::queryTexture(const std::string &name)
{
    return m_texture_registry->GetMetadata(name) != nullptr;
}
