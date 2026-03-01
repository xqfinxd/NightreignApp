#include "AsyncResourceLoader.h"
#include "Texture.h"
#include "TextureRegistry.h"
#include <SDL_log.h>
#include <stdio.h>
#include <stb_image.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <sys/stat.h>
#include <errno.h>
#endif

AsyncResourceLoader::AsyncResourceLoader()
{
    stbi_set_flip_vertically_on_load(1);
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Initialized");
}

AsyncResourceLoader::~AsyncResourceLoader()
{
    m_loading_requests.clear();
    m_update_requests.clear();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Destroyed");
}

#ifdef __EMSCRIPTEN__
static bool createDirectories(const std::string& path) {
    size_t pos = 0;
    std::string dir;

    if (!path.empty() && path[0] == '/') {
        pos = 1;
    }
    
    while (pos != std::string::npos) {
        pos = path.find('/', pos + 1);
        dir = path.substr(0, pos);
        
        if (!dir.empty()) {
            struct stat st;
            if (stat(dir.c_str(), &st) != 0) {
                if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create directory: %s (errno: %d)", dir.c_str(), errno);
                    return false;
                }
            }
        }
    }
    
    return true;
}
#endif

bool AsyncResourceLoader::isFileCached(const std::string& path)
{
#ifdef __EMSCRIPTEN__
    FILE* f = fopen(path.c_str(), "rb");
    if (f) {
        fclose(f);
        return true;
    }
#endif
    return false;
}

void AsyncResourceLoader::syncCache()
{
#ifdef __EMSCRIPTEN__
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Syncing cache to IndexedDB...");
    EM_ASM(
        FS.syncfs(false, function(err) {
            if (err) {
                console.error('Failed to save cache to IndexedDB:', err);
            } else {
                console.log('Cache saved to IndexedDB successfully');
            }
        });
    );
#endif
}

void AsyncResourceLoader::loadTextureDataAsync(const std::string& alias, TextureRegistry* registry, UpdateCallback callback)
{
    if (!registry) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Invalid registry pointer");
        if (callback) callback(false);
        return;
    }
    
    const TextureMetadata* metadata = registry->GetMetadata(alias);
    if (!metadata) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture alias not found in registry: %s", alias.c_str());
        if (callback) callback(false);
        return;
    }
    
    std::string realPath = metadata->path;
    
    if (m_update_requests.find(alias) != m_update_requests.end()) {
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture data '%s' is already loading", alias.c_str());
        return;
    }
    
#ifdef __EMSCRIPTEN__
    std::string filePath = "/" + realPath;
    if (isFileCached(filePath)) {
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Loading cached texture data '%s' (%s)", alias.c_str(), realPath.c_str());
        
        int width, height, channels;
        unsigned char* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
        
        bool success = false;
        if (pixels) {
            size_t dataSize = width * height * channels;
            success = registry->UpdateTexture(alias, pixels, dataSize);
            
            if (success) {
                SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Cached texture loaded: '%s' (%dx%d, %d channels)",
                        alias.c_str(), width, height, channels);
            }
            
            stbi_image_free(pixels);
        }
        
        if (callback) callback(success);
        return;
    }

    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Downloading texture data '%s' from '%s'", alias.c_str(), realPath.c_str());
    
    UpdateRequest request;
    request.alias = alias;
    request.path = realPath;
    request.registry = registry;
    request.callback = callback;
    m_update_requests[alias] = request;
    m_pending_count++;
    
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = onTextureDataDownloadSuccess;
    attr.onerror = onTextureDataDownloadError;
    attr.userData = this;

    emscripten_fetch(&attr, realPath.c_str());
#else
    bool success = false;

    int width, height, channels;
    unsigned char* pixels = stbi_load(realPath.c_str(), &width, &height, &channels, 0);
    if (pixels) {
        size_t dataSize = width * height * channels;
        success = registry->UpdateTexture(alias, pixels, dataSize);

        if (success) {
            SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Cached texture loaded: '%s' (%dx%d, %d channels)",
                alias.c_str(), width, height, channels);
        }

        stbi_image_free(pixels);
    }
    if (callback) callback(success);
    return;
#endif
}

#ifdef __EMSCRIPTEN__
void AsyncResourceLoader::onTextureDataDownloadSuccess(emscripten_fetch_t* fetch)
{
    AsyncResourceLoader* loader = static_cast<AsyncResourceLoader*>(fetch->userData);
    
    std::string url = fetch->url;
    
    std::string downloadPath = url;
    size_t pos = downloadPath.find("/nightreign/");
    if (pos != std::string::npos) {
        downloadPath = downloadPath.substr(pos + 12); // strlen("/nightreign/") = 12
    }
    
    std::string alias;
    UpdateRequest request;
    bool found = false;
    
    for (auto& pair : loader->m_update_requests) {
        if (pair.second.path == downloadPath) {
            alias = pair.first;
            request = pair.second;
            found = true;
            break;
        }
    }
    
    if (!found) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Unknown texture data download: %s", url.c_str());
        emscripten_fetch_close(fetch);
        return;
    }
    
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture data downloaded: '%s' (%s) (%llu bytes)", alias.c_str(), downloadPath.c_str(), fetch->numBytes);
    
    bool success = false;
    std::string filePath = "/" + downloadPath;
    size_t lastSlash = filePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string directory = filePath.substr(0, lastSlash);
        createDirectories(directory);
    }
    
    FILE* f = fopen(filePath.c_str(), "wb");
    if (f) {
        fwrite(fetch->data, 1, fetch->numBytes, f);
        fclose(f);
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Saved texture file to IDBFS: %s", filePath.c_str());
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Failed to save texture file: %s", filePath.c_str());
    }
    
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(fetch->data),
        fetch->numBytes,
        &width, &height, &channels, 0
    );
    
    if (pixels) {
        size_t dataSize = width * height * channels;
        success = request.registry->UpdateTexture(alias, pixels, dataSize);
        
        if (success) {
            SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture updated successfully: '%s' (%dx%d, %d channels)",
                    alias.c_str(), width, height, channels);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Failed to update texture: %s", alias.c_str());
        }
        stbi_image_free(pixels);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Failed to decode image data: %s", alias.c_str());
    }
    
    if (success) {
        loader->syncCache();
    }
    
    if (request.callback) {
        request.callback(success);
    }
    
    loader->m_update_requests.erase(alias);
    loader->m_pending_count--;
    emscripten_fetch_close(fetch);
}

void AsyncResourceLoader::onTextureDataDownloadError(emscripten_fetch_t* fetch)
{
    AsyncResourceLoader* loader = static_cast<AsyncResourceLoader*>(fetch->userData);
    
    std::string url = fetch->url;
    std::string downloadPath = url;
    size_t pos = downloadPath.find("/nightreign/");
    if (pos != std::string::npos) {
        downloadPath = downloadPath.substr(pos + 12);
    }
    
    std::string alias;
    UpdateRequest request;
    bool found = false;
    
    for (auto& pair : loader->m_update_requests) {
        if (pair.second.path == downloadPath) {
            alias = pair.first;
            request = pair.second;
            found = true;
            break;
        }
    }
    
    if (found) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture data download failed: '%s' (%s) (Status: %d)",
                     alias.c_str(), downloadPath.c_str(), fetch->status);
        
        if (request.callback) {
            request.callback(false);
        }
        
        loader->m_update_requests.erase(alias);
    }
    
    loader->m_pending_count--;
    emscripten_fetch_close(fetch);
}
#endif
