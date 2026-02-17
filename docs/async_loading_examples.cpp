// 异步资源加载示例
// 这个文件展示了如何在项目中使用 AsyncResourceLoader

#include "AsyncResourceLoader.h"
#include "ResourceManager.h"
#include "Texture.h"
#include <vector>
#include <string>

// ============================================================================
// 示例 1: 在 Scene 中异步加载地图纹理
// ============================================================================

class MapScene {
private:
    std::vector<std::string> m_requiredTextures;
    std::vector<std::string> m_loadedTextures;
    int m_currentMapId = 0;
    bool m_allTexturesLoaded = false;
    
public:
    void loadMap(int mapId) {
        m_currentMapId = mapId;
        m_allTexturesLoaded = false;
        m_loadedTextures.clear();
        m_requiredTextures.clear();
        
        // 获取异步加载器
        auto loader = ResourceManager::getInstance()->getAsyncLoader();
        
        // 加载该地图的所有纹理瓦片
        for (int layer = 0; layer <= 1; layer++) {
            for (int x = 0; x < 6; x++) {
                for (int y = 0; y < 6; y++) {
                    char nameBuf[128];
                    char pathBuf[256];
                    
                    snprintf(nameBuf, sizeof(nameBuf), 
                            "map_%d_L%d_%02d_%02d", mapId, layer, x, y);
                    snprintf(pathBuf, sizeof(pathBuf), 
                            "nightreign/assets/textures/%d/MENU_MapTile_L%d_%02d_%02d.png",
                            mapId, layer, x, y);
                    
                    m_requiredTextures.push_back(nameBuf);
                    
                    // 异步加载纹理
                    loader->loadTextureAsync(nameBuf, pathBuf, 
                        [this, texName = std::string(nameBuf)](bool success, Texture* texture) {
                            if (success) {
                                m_loadedTextures.push_back(texName);
                                onTextureLoaded(texName, texture);
                            } else {
                                onTextureLoadFailed(texName);
                            }
                        });
                }
            }
        }
    }
    
    void onTextureLoaded(const std::string& name, Texture* texture) {
        SDL_Log("Texture loaded: %s", name.c_str());
        
        // 检查是否所有纹理都已加载
        if (m_loadedTextures.size() >= m_requiredTextures.size()) {
            m_allTexturesLoaded = true;
            onAllTexturesLoaded();
        }
    }
    
    void onTextureLoadFailed(const std::string& name) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load texture: %s", name.c_str());
    }
    
    void onAllTexturesLoaded() {
        SDL_Log("All textures loaded for map %d", m_currentMapId);
        // 可以开始渲染地图了
    }
    
    void update(float deltaTime) {
        // 显示加载进度
        if (!m_allTexturesLoaded) {
            auto loader = ResourceManager::getInstance()->getAsyncLoader();
            int pending = loader->getPendingCount();
            // 可以在这里更新 UI 显示加载进度
        }
    }
};

// ============================================================================
// 示例 2: 预加载下一个地图的纹理（后台加载）
// ============================================================================

class MapPreloader {
private:
    int m_nextMapId = -1;
    bool m_isPreloading = false;
    
public:
    void preloadNextMap(int nextMapId) {
        if (m_isPreloading) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Already preloading a map");
            return;
        }
        
        m_nextMapId = nextMapId;
        m_isPreloading = true;
        
        auto loader = ResourceManager::getInstance()->getAsyncLoader();
        
        // 静默预加载（不显示加载界面）
        for (int x = 0; x < 6; x++) {
            for (int y = 0; y < 6; y++) {
                char nameBuf[128];
                char pathBuf[256];
                
                snprintf(nameBuf, sizeof(nameBuf), "map_%d_L0_%02d_%02d", nextMapId, x, y);
                snprintf(pathBuf, sizeof(pathBuf), 
                        "nightreign/assets/textures/%d/MENU_MapTile_L0_%02d_%02d.png",
                        nextMapId, x, y);
                
                loader->loadTextureAsync(nameBuf, pathBuf, nullptr); // 不需要回调
            }
        }
        
        SDL_Log("Started preloading map %d in background", nextMapId);
    }
};

// ============================================================================
// 示例 3: 检查缓存并智能加载
// ============================================================================

class SmartResourceLoader {
public:
    void loadTextureWithFallback(const std::string& name, const std::string& path) {
        auto resourceMgr = ResourceManager::getInstance();
        auto loader = resourceMgr->getAsyncLoader();
        
        // 先检查是否已经加载
        Texture* existing = resourceMgr->getTexture(name);
        if (existing) {
            SDL_Log("Texture '%s' already loaded", name.c_str());
            return;
        }
        
        // 检查是否已缓存
        if (loader->isFileCached(path)) {
            SDL_Log("Texture '%s' found in cache, loading synchronously", name.c_str());
            resourceMgr->loadTexture(name, path);
        } else {
            SDL_Log("Texture '%s' not cached, downloading asynchronously", name.c_str());
            loader->loadTextureAsync(name, path, 
                [name](bool success, Texture* texture) {
                    if (success) {
                        SDL_Log("Texture '%s' downloaded and cached", name.c_str());
                    }
                });
        }
    }
};

// ============================================================================
// 示例 4: 批量加载 Spot 图标
// ============================================================================

class SpotIconLoader {
private:
    std::vector<std::string> m_spotIcons = {
        "boss", "buried treasure", "cabin", "cart", "castle", "cave",
        "church", "danger boss", "encampment", "evergaol", "fort",
        "grace", "launch", "play area", "ruins", "smallcamp",
        "sorcerer's rise", "target", "task", "temple", "township",
        "undefined", "village"
    };
    
    int m_loadedCount = 0;
    bool m_allLoaded = false;
    
public:
    void loadAllSpotIcons() {
        m_loadedCount = 0;
        m_allLoaded = false;
        
        auto loader = ResourceManager::getInstance()->getAsyncLoader();
        
        for (const auto& iconName : m_spotIcons) {
            std::string texName = "spot_" + iconName;
            std::string texPath = "nightreign/assets/textures/spots/" + iconName + ".png";
            
            loader->loadTextureAsync(texName, texPath,
                [this, texName](bool success, Texture* texture) {
                    if (success) {
                        m_loadedCount++;
                        
                        if (m_loadedCount >= m_spotIcons.size()) {
                            m_allLoaded = true;
                            SDL_Log("All spot icons loaded (%d)", m_loadedCount);
                        }
                    }
                });
        }
    }
    
    bool isReady() const {
        return m_allLoaded;
    }
    
    float getProgress() const {
        return static_cast<float>(m_loadedCount) / m_spotIcons.size();
    }
};

// ============================================================================
// 示例 5: 手动触发缓存同步
// ============================================================================

class CacheManager {
public:
    void syncCacheNow() {
        auto loader = ResourceManager::getInstance()->getAsyncLoader();
        loader->syncCache();
        SDL_Log("Cache sync triggered manually");
    }
    
    void onApplicationPause() {
        // 应用暂停时保存缓存（如切换到后台）
        syncCacheNow();
    }
    
    void onLevelComplete() {
        // 完成一个关卡后保存缓存
        syncCacheNow();
    }
};
