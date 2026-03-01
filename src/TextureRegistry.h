#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "public.h"
#include "Texture.h"
#include <functional>

class AsyncResourceLoader;

/**
 * 合成图层：将一张源纹理按指定位置/缩放叠加到合成纹理画布上。
 *
 * 坐标系(与 OpenGL UV / stbi_set_flip_vertically_on_load 一致)：
 *   (0, 0) = 画布/纹理视觉左下角
 *   (1, 1) = 画布/纹理视觉右上角
 *
 *   anchor : 目标画布上的锚点位置(归一化)
 *   pivot  : 源纹理上的锚点位置(归一化)，该点会对齐到 anchor
 *   scale  : 源纹理相对于原始尺寸的缩放倍数
 *
 * 常用预设：
 *   右下叠加 → anchor=(1,0), pivot=(1,0)
 *   左上叠加 → anchor=(0,1), pivot=(0,1)
 *   居中叠加 → anchor=(0.5,0.5), pivot=(0.5,0.5)
 */
struct CompositeLayer {
    std::string sourceAlias;    // 源纹理别名(必须在 atlas.csv 中已注册)
    float anchorX = 0.0f;       // 画布锚点 X
    float anchorY = 0.0f;       // 画布锚点 Y
    float pivotX = 0.0f;       // 源图轴心 X
    float pivotY = 0.0f;       // 源图轴心 Y
    float scale = 1.0f;       // 缩放
};

// 纹理元数据结构
struct TextureMetadata {
    std::string alias;  // 别名(用于查找)
    std::string path;   // 相对路径(用于下载)
    std::vector<CompositeLayer> layers;
    int width;          // 宽度
    int height;         // 高度
    int channel;        // 像素格式
    GLuint textureId;   // OpenGL纹理ID(0表示未创建)
    bool isLoaded;      // 是否已加载真实数据
    
    TextureMetadata() 
        : width(0), height(0), channel(4)
        , textureId(0), isLoaded(false) 
    {}
};

/**
 * 纹理注册表 - 管理所有纹理的元数据和占位纹理
 */
class TextureRegistry {
public:
    TextureRegistry();
    ~TextureRegistry();
    
    void loadTextureDataAsync(const std::string& alias, std::function<void(bool success)> callback);

    /**
     * 从atlas.csv加载纹理元数据
     * @param atlasPath atlas.csv文件路径
     * @return 成功返回true
     */
    bool LoadAtlas(const std::string& atlasPath);
    bool loadAtlasJson(const std::string& jsonPath);

    /**
     * 从 composite_atlas.csv 加载合成纹理定义。
     * 每个合成别名会作为占位元数据注册到 m_textures，以便 queryTexture / loadTexture 正常工作。
     * @param csvPath composite_atlas.csv 文件路径
     * @return 成功返回 true(文件不存在时返回 true 但不加载)
     */
    bool LoadCompositeAtlas(const std::string& csvPath);
    bool loadCompositeAtlasJson(const std::string& jsonPath);

    /**
     * 对所有已定义的合成纹理执行 GPU bake(需要有效 OpenGL 上下文)。
     * 所有源纹理必须先通过 loadTexture 载入。
     * @return 成功 bake 的数量
     */
    int BakeCompositeTextures();

    /**
     * 对单个合成纹理执行 GPU bake。
     * 若任意源纹理尚未载入(textureId == 0)，则本次跳过并返回 false。
     * @param alias 合成纹理别名
     * @return 成功返回 true
     */
    bool BakeComposite(const std::string& alias);
    
    /**
     * 为指定别名创建占位纹理 (1x1白色纹理)
     * 必须在OpenGL上下文中调用
     * @param alias 纹理别名
     * @return 成功返回纹理ID，失败或不存在返回0
     */
    GLuint CreatePlaceholderForAlias(const std::string& alias);
    
    /**
     * 预创建所有占位纹理 (1x1白色纹理) - 已弃用，建议使用按需创建
     * 必须在OpenGL上下文中调用
     */
    void CreatePlaceholderTextures();
    
    /**
     * 根据别名获取纹理元数据
     * @param alias 纹理别名
     * @return 元数据指针，不存在返回nullptr
     */
    const TextureMetadata* GetMetadata(const std::string& alias) const;
    
    /**
     * 获取纹理ID (可能是占位纹理或真实纹理)
     * @param alias 纹理别名
     * @return OpenGL纹理ID，不存在返回0
     */
    GLuint GetTextureId(const std::string& alias) const;
    
    /**
     * 更新纹理数据 (当异步加载完成时调用)。
     * 若该纹理是某合成纹理的源之一，且其他源均已就绪，则自动 re-bake 合成纹理。
     * @param path 纹理路径
     * @param pixels 像素数据
     * @param dataSize 数据大小
     * @return 成功返回true
     */
    bool UpdateTexture(const std::string& alias, const void* pixels, size_t dataSize);
    
    /**
     * 获取所有纹理别名列表
     */
    std::vector<std::string> GetAllTextureAliases() const;
    
    /**
     * 获取纹理总数
     */
    size_t GetTextureCount() const { return m_textures.size(); }
    
    /**
     * 获取已加载纹理数量
     */
    size_t GetLoadedCount() const;
    
    /**
     * 清理所有纹理
     */
    void Clear();
    
private:
    // 获取OpenGL格式
    GLenum GetGLFormat(int format) const;
    
    // 创建单个占位纹理
    GLuint CreatePlaceholderTexture(int width, int height, int format);

    // 初始化 FBO bake 所需的 GLSL 程序和 VAO/VBO(幂等，可重复调用)
    bool InitBakePipeline();

    // 销毁 bake pipeline 资源
    void DestroyBakePipeline();
    
private:
    AsyncResourceLoader* m_async_loader = nullptr;
    // 按别名索引(含合成占位)
    std::unordered_map<std::string, TextureMetadata>    m_textures;     
    // source → composite 反向索引，用于 UpdateTexture 后的自动 re-bake
    std::unordered_map<std::string, std::vector<std::string>> m_compositesBySource;

    // Bake pipeline(懒初始化)
    GLuint m_bakeProgram = 0;
    GLuint m_bakeVAO     = 0;
    GLuint m_bakeVBO     = 0;
};
