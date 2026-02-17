#include "AsyncResourceLoader.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "TextureRegistry.h"
#include <SDL_log.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <stb_image.h>
#include <sys/stat.h>
#include <errno.h>
#endif

AsyncResourceLoader::AsyncResourceLoader(ResourceManager* resourceMgr)
    : m_resource_mgr(resourceMgr)
{
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Initialized");
}

AsyncResourceLoader::~AsyncResourceLoader()
{
    m_loading_requests.clear();
    m_update_requests.clear();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Destroyed");
}

#ifdef __EMSCRIPTEN__
// 辅助函数：递归创建目录
static bool createDirectories(const std::string& path) {
    size_t pos = 0;
    std::string dir;
    
    // 跳过开头的斜杠
    if (!path.empty() && path[0] == '/') {
        pos = 1;
    }
    
    while (pos != std::string::npos) {
        pos = path.find('/', pos + 1);
        dir = path.substr(0, pos);
        
        if (!dir.empty()) {
            struct stat st;
            if (stat(dir.c_str(), &st) != 0) {
                // 目录不存在，创建它
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

void AsyncResourceLoader::loadTextureDataAsync(const std::string& path, TextureRegistry* registry, UpdateCallback callback)
{
    if (!registry) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Invalid registry pointer");
        if (callback) callback(false);
        return;
    }
    
    // 检查是否正在加载
    if (m_update_requests.find(path) != m_update_requests.end()) {
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture data '%s' is already loading", path.c_str());
        return;
    }
    
#ifdef __EMSCRIPTEN__
    // 检查文件是否已缓存
    std::string filePath = "/" + path;
    if (isFileCached(filePath)) {
        SDL_Log("AsyncResourceLoader: Loading cached texture data '%s'", path.c_str());
        
        // 从缓存加载图片
        int width, height, channels;
        unsigned char* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
        
        bool success = false;
        if (pixels) {
            size_t dataSize = width * height * channels;
            success = registry->UpdateTexture(path, pixels, dataSize);
            
            if (success) {
                SDL_Log("AsyncResourceLoader: Cached texture loaded: '%s' (%dx%d, %d channels)",
                        path.c_str(), width, height, channels);
            }
            
            stbi_image_free(pixels);
        }
        
        if (callback) callback(success);
        return;
    }
    
    // 异步下载纹理数据
    SDL_Log("AsyncResourceLoader: Downloading texture data '%s'", path.c_str());
    
    UpdateRequest request;
    request.path = path;
    request.registry = registry;
    request.callback = callback;
    m_update_requests[path] = request;
    m_pending_count++;
    
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = onTextureDataDownloadSuccess;
    attr.onerror = onTextureDataDownloadError;
    attr.userData = this;
    
    // 构建完整URL
    std::string url = path;
    emscripten_fetch(&attr, url.c_str());
#else
    // 非Emscripten环境，暂不支持
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Async texture update not supported in non-Emscripten builds");
    if (callback) callback(false);
#endif
}

#ifdef __EMSCRIPTEN__
void AsyncResourceLoader::onTextureDataDownloadSuccess(emscripten_fetch_t* fetch)
{
    AsyncResourceLoader* loader = static_cast<AsyncResourceLoader*>(fetch->userData);
    
    // 查找对应的请求
    std::string url = fetch->url;
    
    // 移除URL前缀 "/nightreign/"
    std::string path = url;
    size_t pos = path.find("/nightreign/");
    if (pos != std::string::npos) {
        path = path.substr(pos + 12); // strlen("/nightreign/") = 12
    }
    
    auto it = loader->m_update_requests.find(path);
    if (it == loader->m_update_requests.end()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Unknown texture data download: %s", url.c_str());
        emscripten_fetch_close(fetch);
        return;
    }
    
    UpdateRequest& request = it->second;
    SDL_Log("AsyncResourceLoader: Texture data downloaded: '%s' (%llu bytes)", path.c_str(), fetch->numBytes);
    
    bool success = false;
    
    // 先保存原始PNG数据到虚拟文件系统（用于下次启动时缓存）
    std::string filePath = "/" + path;  // 添加根路径前缀
    
    // 确保目录结构存在
    size_t lastSlash = filePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string directory = filePath.substr(0, lastSlash);
        createDirectories(directory);
    }
    
    FILE* f = fopen(filePath.c_str(), "wb");
    if (f) {
        fwrite(fetch->data, 1, fetch->numBytes, f);
        fclose(f);
        SDL_Log("AsyncResourceLoader: Saved texture file to IDBFS: %s", filePath.c_str());
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Failed to save texture file: %s", filePath.c_str());
    }
    
    // 使用stb_image解码图片数据
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(
        reinterpret_cast<const unsigned char*>(fetch->data),
        fetch->numBytes,
        &width, &height, &channels, 0
    );
    
    if (pixels) {
        // 计算数据大小
        size_t dataSize = width * height * channels;
        
        // 更新纹理
        success = request.registry->UpdateTexture(path, pixels, dataSize);
        
        if (success) {
            SDL_Log("AsyncResourceLoader: Texture updated successfully: '%s' (%dx%d, %d channels)",
                    path.c_str(), width, height, channels);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Failed to update texture: %s", path.c_str());
        }
        
        // 释放stb_image分配的内存
        stbi_image_free(pixels);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Failed to decode image data: %s", path.c_str());
    }
    
    // 定期同步到IndexedDB持久化
    if (success) {
        loader->syncCache();
    }
    
    // 触发回调
    if (request.callback) {
        request.callback(success);
    }
    
    loader->m_update_requests.erase(it);
    loader->m_pending_count--;
    emscripten_fetch_close(fetch);
}

void AsyncResourceLoader::onTextureDataDownloadError(emscripten_fetch_t* fetch)
{
    AsyncResourceLoader* loader = static_cast<AsyncResourceLoader*>(fetch->userData);
    
    std::string url = fetch->url;
    std::string path = url;
    size_t pos = path.find("/nightreign/");
    if (pos != std::string::npos) {
        path = path.substr(pos + 12);
    }
    
    auto it = loader->m_update_requests.find(path);
    if (it != loader->m_update_requests.end()) {
        UpdateRequest& request = it->second;
        
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "AsyncResourceLoader: Texture data download failed: '%s' (Status: %d)",
                     path.c_str(), fetch->status);
        
        if (request.callback) {
            request.callback(false);
        }
        
        loader->m_update_requests.erase(it);
    }
    
    loader->m_pending_count--;
    emscripten_fetch_close(fetch);
}
#endif
