#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "public.h"
#include "Texture.h"

// 纹理元数据结构
struct TextureMetadata {
    std::string alias;          // 别名（用于查找）
    std::string path;           // 相对路径（用于下载）
    int width;                  // 宽度
    int height;                 // 高度
    TextureFormat format;       // 像素格式
    GLuint textureId;          // OpenGL纹理ID (0表示未创建)
    bool isLoaded;             // 是否已加载真实数据
    
    TextureMetadata() 
        : width(0), height(0), format(TextureFormat::Unknown)
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
    
    /**
     * 从atlas.csv加载纹理元数据
     * @param atlasPath atlas.csv文件路径
     * @return 成功返回true
     */
    bool LoadAtlas(const std::string& atlasPath);
    
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
     * 更新纹理数据 (当异步加载完成时调用)
     * @param path 纹理路径
     * @param pixels 像素数据
     * @param dataSize 数据大小
     * @return 成功返回true
     */
    bool UpdateTexture(const std::string& path, const void* pixels, size_t dataSize);
    
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
    // 解析格式字符串
    TextureFormat ParseFormat(const std::string& formatStr) const;
    
    // 获取OpenGL格式
    GLenum GetGLFormat(TextureFormat format) const;
    
    // 创建单个占位纹理
    GLuint CreatePlaceholderTexture(int width, int height, TextureFormat format);
    
private:
    std::unordered_map<std::string, TextureMetadata> m_textures;  // 按别名索引
};
