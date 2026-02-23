#include "TextureRegistry.h"
#include "CsvReader.h"
#include <iostream>
#include <cstring>
#include <SDL.h>

#include "generated/AtlasRow.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

TextureRegistry::TextureRegistry() {
}

TextureRegistry::~TextureRegistry() {
    Clear();
}

bool TextureRegistry::LoadAtlas(const std::string& atlasPath) {
    auto rowdata = readCSVFile<AtlasRow>(atlasPath);
    
    if (rowdata.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Atlas is empty: %s", atlasPath.c_str());
        return false;
    }
    
    // alias,path,width,height,format
    int loadedCount = 0;
    for (size_t i = 0; i < rowdata.size(); ++i) {
        TextureMetadata meta;
        meta.alias = rowdata[i].alias;
        meta.path = rowdata[i].path;
        meta.width = rowdata[i].width;
        meta.height = rowdata[i].height;
        meta.format = ParseFormat(rowdata[i].format);
        
        if (meta.width <= 0 || meta.height <= 0) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Invalid dimensions for %s", meta.alias.c_str());
            continue;
        }
        
        m_textures[meta.alias] = meta;
        loadedCount++;
    }
    
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Loaded %d texture metadata entries", loadedCount);
    return loadedCount > 0;
}

GLuint TextureRegistry::CreatePlaceholderForAlias(const std::string& alias) {
    auto it = m_textures.find(alias);
    if (it == m_textures.end()) {
        return 0;
    }
    
    TextureMetadata& meta = it->second;

    if (meta.textureId != 0) {
        return meta.textureId;
    }

    meta.textureId = CreatePlaceholderTexture(meta.width, meta.height, meta.format);
    
    return meta.textureId;
}

void TextureRegistry::CreatePlaceholderTextures() {
    int createdCount = 0;
    for (auto& pair : m_textures) {
        TextureMetadata& meta = pair.second;

        if (meta.textureId != 0) {
            continue;
        }

        meta.textureId = CreatePlaceholderTexture(meta.width, meta.height, meta.format);
        
        if (meta.textureId != 0) {
            createdCount++;
        }
    }
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Texture not found: %s", alias.c_str());
        return false;
    }
    
    TextureMetadata& meta = it->second;
    
    if (meta.textureId == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Texture ID not created: %s", alias.c_str());
        return false;
    }
    
    int channels = 4;
    switch (meta.format) {
        case TextureFormat::R: channels = 1; break;
        case TextureFormat::RGB: channels = 3; break;
        case TextureFormat::RGBA: channels = 4; break;
        default: channels = 4;
    }
    
    size_t expectedSize = meta.width * meta.height * channels;
    if (dataSize != expectedSize) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Data size mismatch for %s (expected %zu, got %zu)",
            alias.c_str(), expectedSize, dataSize);
        return false;
    }
    
    glBindTexture(GL_TEXTURE_2D, meta.textureId);
    
    GLenum format = GetGLFormat(meta.format);
    glTexImage2D(GL_TEXTURE_2D, 0, format, meta.width, meta.height, 0, 
                 format, GL_UNSIGNED_BYTE, pixels);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    meta.isLoaded = true;
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
    
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Unknown format: %s, using RGBA as default", formatStr.c_str());
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Failed to generate texture ID");
        return 0;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureId);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    
    GLenum glFormat = GetGLFormat(format);
    
    glTexImage2D(GL_TEXTURE_2D, 0, glFormat, 1, 1, 0, 
                 glFormat, GL_UNSIGNED_BYTE, whitePixel);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureId;
}
