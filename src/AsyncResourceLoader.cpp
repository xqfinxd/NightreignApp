#include "AsyncResourceLoader.h"
#include "ResourceManager.h"
#include "Texture.h"
#include <SDL_log.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

AsyncResourceLoader::AsyncResourceLoader(ResourceManager* resourceMgr)
    : m_resource_mgr(resourceMgr)
{
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Initialized");
}

AsyncResourceLoader::~AsyncResourceLoader()
{
    m_loading_requests.clear();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Destroyed");
}

bool AsyncResourceLoader::isFileCached(const std::string& path)
{
#ifdef __EMSCRIPTEN__
    // 检查文件是否存在于虚拟文件系统中
    FILE* f = fopen(path.c_str(), "rb");
    if (f) {
        fclose(f);
        return true;
    }
#endif
    return false;
}

void AsyncResourceLoader::loadTextureAsync(const std::string& name, const std::string& path, LoadCallback callback)
{
    // 检查是否已经加载
    Texture* existing = m_resource_mgr->getTexture(name);
    if (existing) {
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture '%s' already loaded", name.c_str());
        if (callback) {
            callback(true, existing);
        }
        return;
    }
    
    // 检查是否正在加载
    if (m_loading_requests.find(name) != m_loading_requests.end()) {
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture '%s' is already loading", name.c_str());
        return;
    }
    
    // 如果文件已缓存，直接加载
    if (isFileCached(path)) {
        SDL_Log("AsyncResourceLoader: Loading cached texture '%s' from '%s'", name.c_str(), path.c_str());
        Texture* texture = m_resource_mgr->loadTexture(name, path);
        if (callback) {
            callback(texture != nullptr, texture);
        }
        return;
    }
    
#ifdef __EMSCRIPTEN__
    // 文件未缓存，需要异步下载
    SDL_Log("AsyncResourceLoader: Downloading texture '%s' from '%s'", name.c_str(), path.c_str());
    
    LoadRequest request;
    request.name = name;
    request.path = path;
    request.callback = callback;
    m_loading_requests[name] = request;
    m_pending_count++;
    
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = onFileDownloadSuccess;
    attr.onerror = onFileDownloadError;
    attr.userData = this;
    
    emscripten_fetch(&attr, path.c_str());
#else
    // 非 Emscripten 环境，直接同步加载
    Texture* texture = m_resource_mgr->loadTexture(name, path);
    if (callback) {
        callback(texture != nullptr, texture);
    }
#endif
}

void AsyncResourceLoader::syncCache()
{
#ifdef __EMSCRIPTEN__
    SDL_Log("AsyncResourceLoader: Syncing cache to IndexedDB...");
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

#ifdef __EMSCRIPTEN__
void AsyncResourceLoader::onFileDownloadSuccess(emscripten_fetch_t* fetch)
{
    AsyncResourceLoader* loader = static_cast<AsyncResourceLoader*>(fetch->userData);
    
    // 查找对应的请求
    std::string url = fetch->url;
    std::string name;
    LoadRequest request;
    bool found = false;
    
    for (auto& pair : loader->m_loading_requests) {
        if (pair.second.path == url) {
            name = pair.first;
            request = pair.second;
            found = true;
            break;
        }
    }
    
    if (!found) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Unknown download completed: %s", url.c_str());
        emscripten_fetch_close(fetch);
        return;
    }
    
    SDL_Log("AsyncResourceLoader: Download completed for '%s' (%llu bytes)", name.c_str(), fetch->numBytes);
    
    // 保存到虚拟文件系统
    FILE* f = fopen(request.path.c_str(), "wb");
    if (f) {
        fwrite(fetch->data, 1, fetch->numBytes, f);
        fclose(f);
        
        // 加载纹理
        Texture* texture = loader->m_resource_mgr->loadTexture(name, request.path);
        
        // 触发回调
        if (request.callback) {
            request.callback(texture != nullptr, texture);
        }
        
        // 定期保存到 IndexedDB
        loader->syncCache();
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Failed to save file '%s'", request.path.c_str());
        if (request.callback) {
            request.callback(false, nullptr);
        }
    }
    
    loader->m_loading_requests.erase(name);
    loader->m_pending_count--;
    emscripten_fetch_close(fetch);
}

void AsyncResourceLoader::onFileDownloadError(emscripten_fetch_t* fetch)
{
    AsyncResourceLoader* loader = static_cast<AsyncResourceLoader*>(fetch->userData);
    
    std::string url = fetch->url;
    std::string name;
    LoadRequest request;
    
    for (auto& pair : loader->m_loading_requests) {
        if (pair.second.path == url) {
            name = pair.first;
            request = pair.second;
            break;
        }
    }
    
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Download failed for '%s' (Status: %d)", url.c_str(), fetch->status);
    
    if (request.callback) {
        request.callback(false, nullptr);
    }
    
    loader->m_loading_requests.erase(name);
    loader->m_pending_count--;
    emscripten_fetch_close(fetch);
}
#endif
