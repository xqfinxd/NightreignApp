#pragma once
#include "public.h"
#include <string>
#include <functional>
#include <unordered_map>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

class Texture;
class ResourceManager;
class TextureRegistry;

// 异步资源加载器 - 简化版本
class AsyncResourceLoader
{
public:
    using LoadCallback = std::function<void(bool success, Texture* texture)>;
    using UpdateCallback = std::function<void(bool success)>;
    
    AsyncResourceLoader(ResourceManager* resourceMgr);
    ~AsyncResourceLoader();
    
    // 异步加载纹理，如果已缓存则直接返回
    void loadTextureAsync(const std::string& name, const std::string& path, LoadCallback callback = nullptr);
    
    // 异步加载纹理数据并更新已存在的占位纹理
    void loadTextureDataAsync(const std::string& path, TextureRegistry* registry, UpdateCallback callback = nullptr);
    
    // 检查文件是否已在 IDBFS 中缓存
    bool isFileCached(const std::string& path);
    
    // 同步 IDBFS 到 IndexedDB（保存缓存）
    void syncCache();
    
    // 获取正在加载的资源数量
    int getPendingCount() const { return m_pending_count; }
    
private:
    struct LoadRequest {
        std::string name;
        std::string path;
        LoadCallback callback;
    };
    
    struct UpdateRequest {
        std::string path;
        TextureRegistry* registry;
        UpdateCallback callback;
    };
    
    ResourceManager* m_resource_mgr;
    std::unordered_map<std::string, LoadRequest> m_loading_requests;
    std::unordered_map<std::string, UpdateRequest> m_update_requests;
    int m_pending_count = 0;
    
#ifdef __EMSCRIPTEN__
    static void onFileDownloadSuccess(emscripten_fetch_t* fetch);
    static void onFileDownloadError(emscripten_fetch_t* fetch);
    static void onTextureDataDownloadSuccess(emscripten_fetch_t* fetch);
    static void onTextureDataDownloadError(emscripten_fetch_t* fetch);
#endif
};
