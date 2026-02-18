#include "TextureRegistry.h"
#include "CsvReader.h"
#include <iostream>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

TextureRegistry::TextureRegistry() {
}

TextureRegistry::~TextureRegistry() {
    Clear();
}

bool TextureRegistry::LoadAtlas(const std::string& atlasPath) {
    CsvReader csv;
    if (!csv.load(atlasPath, true, ',')) {
        std::cerr << "[TextureRegistry] Failed to load atlas: " << atlasPath << std::endl;
        return false;
    }
    
    // 检查行数（不包括标题行）
    if (csv.getRowCount() < 1) {
        std::cerr << "[TextureRegistry] Atlas is empty: " << atlasPath << std::endl;
        return false;
    }
    
    // 解析每一行：alias,path,width,height,format
    int loadedCount = 0;
    for (size_t i = 0; i < csv.getRowCount(); ++i) {
        auto row = csv.getRow(i);
        if (row.size() < 5) {
            std::cerr << "[TextureRegistry] Invalid row " << i << ": insufficient columns" << std::endl;
            continue;
        }
        
        TextureMetadata meta;
        meta.alias = row[0];   // 别名
        meta.path = row[1];    // 路径
        meta.width = std::stoi(row[2]);
        meta.height = std::stoi(row[3]);
        meta.format = ParseFormat(row[4]);
        
        if (meta.width <= 0 || meta.height <= 0) {
            std::cerr << "[TextureRegistry] Invalid dimensions for " << meta.alias << std::endl;
            continue;
        }
        
        m_textures[meta.alias] = meta;  // 按别名索引
        loadedCount++;
    }
    
    std::cout << "[TextureRegistry] Loaded " << loadedCount << " texture metadata entries" << std::endl;
    return loadedCount > 0;
}

GLuint TextureRegistry::CreatePlaceholderForAlias(const std::string& alias) {
    auto it = m_textures.find(alias);
    if (it == m_textures.end()) {
        // 纹理不在atlas中，返回0
        return 0;
    }
    
    TextureMetadata& meta = it->second;
    
    // 如果已经创建过，直接返回
    if (meta.textureId != 0) {
        return meta.textureId;
    }
    
    // 创建占位纹理
    meta.textureId = CreatePlaceholderTexture(meta.width, meta.height, meta.format);
    
    if (meta.textureId != 0) {
        std::cout << "[TextureRegistry] Created placeholder for: " << alias 
                  << " (" << meta.width << "x" << meta.height << ")" << std::endl;
    }
    
    return meta.textureId;
}

void TextureRegistry::CreatePlaceholderTextures() {
    std::cout << "[TextureRegistry] Creating " << m_textures.size() << " placeholder textures..." << std::endl;
    
    int createdCount = 0;
    for (auto& pair : m_textures) {
        TextureMetadata& meta = pair.second;
        
        // 跳过已创建的
        if (meta.textureId != 0) {
            continue;
        }
        
        // 创建占位纹理 (1x1白色)
        meta.textureId = CreatePlaceholderTexture(meta.width, meta.height, meta.format);
        
        if (meta.textureId != 0) {
            createdCount++;
        }
    }
    
    std::cout << "[TextureRegistry] Created " << createdCount << " placeholder textures" << std::endl;
}

const TextureMetadata* TextureRegistry::GetMetadata(const std::string& alias) const {
    auto it = m_textures.find(alias);
    if (it != m_textures.end()) {
        return &it->second;
    }
    return nullptr;
}

GLuint TextureRegistry::GetTextureId(const std::string& alias) const {
    auto it = m_textures.find(alias);
    if (it != m_textures.end()) {
        return it->second.textureId;
    }
    return 0;
}

bool TextureRegistry::UpdateTexture(const std::string& alias, const void* pixels, size_t dataSize) {
    auto it = m_textures.find(alias);
    if (it == m_textures.end()) {
        std::cerr << "[TextureRegistry] Texture not found: " << alias << std::endl;
        return false;
    }
    
    TextureMetadata& meta = it->second;
    
    if (meta.textureId == 0) {
        std::cerr << "[TextureRegistry] Texture ID not created: " << alias << std::endl;
        return false;
    }
    
    // 计算预期数据大小
    int channels = 4; // 默认RGBA
    switch (meta.format) {
        case TextureFormat::R: channels = 1; break;
        case TextureFormat::RGB: channels = 3; break;
        case TextureFormat::RGBA: channels = 4; break;
        default: channels = 4;
    }
    
    size_t expectedSize = meta.width * meta.height * channels;
    if (dataSize != expectedSize) {
        std::cerr << "[TextureRegistry] Data size mismatch for " << alias 
                  << " (expected " << expectedSize << ", got " << dataSize << ")" << std::endl;
        return false;
    }
    
    // 更新纹理数据
    glBindTexture(GL_TEXTURE_2D, meta.textureId);
    
    GLenum format = GetGLFormat(meta.format);
    glTexImage2D(GL_TEXTURE_2D, 0, format, meta.width, meta.height, 0, 
                 format, GL_UNSIGNED_BYTE, pixels);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    meta.isLoaded = true;
    
    std::cout << "[TextureRegistry] Updated texture: " << alias << std::endl;
    return true;
}

std::vector<std::string> TextureRegistry::GetAllTextureAliases() const {
    std::vector<std::string> paths;
    paths.reserve(m_textures.size());
    
    for (const auto& pair : m_textures) {
        paths.push_back(pair.first);
    }
    
    return paths;
}

size_t TextureRegistry::GetLoadedCount() const {
    size_t count = 0;
    for (const auto& pair : m_textures) {
        if (pair.second.isLoaded) {
            count++;
        }
    }
    return count;
}

void TextureRegistry::Clear() {
    // 删除所有OpenGL纹理
    for (auto& pair : m_textures) {
        if (pair.second.textureId != 0) {
            glDeleteTextures(1, &pair.second.textureId);
        }
    }
    m_textures.clear();
}

TextureFormat TextureRegistry::ParseFormat(const std::string& formatStr) const {
    if (formatStr == "RGBA") return TextureFormat::RGBA;
    if (formatStr == "RGB") return TextureFormat::RGB;
    if (formatStr == "LA" || formatStr == "L") return TextureFormat::R;  // Map grayscale/LA to single channel
    
    std::cerr << "[TextureRegistry] Unknown format: " << formatStr << ", using RGBA as default" << std::endl;
    return TextureFormat::RGBA;  // Default to RGBA
}

GLenum TextureRegistry::GetGLFormat(TextureFormat format) const {
    switch (format) {
        case TextureFormat::RGBA: return GL_RGBA;
        case TextureFormat::RGB: return GL_RGB;
        case TextureFormat::R: return GL_LUMINANCE;  // Use luminance for single channel
        default: return GL_RGBA;
    }
}

GLuint TextureRegistry::CreatePlaceholderTexture(int width, int height, TextureFormat format) {
    GLuint textureId;
    glGenTextures(1, &textureId);
    
    if (textureId == 0) {
        std::cerr << "[TextureRegistry] Failed to generate texture ID" << std::endl;
        return 0;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 创建1x1白色占位纹理
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    
    GLenum glFormat = GetGLFormat(format);
    
    // 注意：这里创建1x1的占位纹理，节省内存
    // 实际纹理加载时会用真实尺寸替换
    glTexImage2D(GL_TEXTURE_2D, 0, glFormat, 1, 1, 0, 
                 glFormat, GL_UNSIGNED_BYTE, whitePixel);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureId;
}
