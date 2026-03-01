#include "TextureRegistry.h"
#include "CsvReader.h"
#include "AsyncResourceLoader.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <SDL.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include "generated/AtlasRow.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

TextureRegistry::TextureRegistry() {
    m_async_loader = new AsyncResourceLoader();
}

TextureRegistry::~TextureRegistry() {
    Clear();
    delete m_async_loader;
    m_async_loader = nullptr;
}

void TextureRegistry::loadTextureDataAsync(const std::string& alias, std::function<void(bool success)> callback)
{
    m_async_loader->loadTextureDataAsync(alias, this, callback);
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
        meta.channel = rowdata[i].format;
        
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

bool TextureRegistry::loadAtlasJson(const std::string& jsonPath)
{
    std::ifstream ifs(jsonPath);
    if (!ifs.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Failed to open atlas JSON: %s", jsonPath.c_str());
        return false;
    }

    rapidjson::IStreamWrapper isw(ifs);

    rapidjson::Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Failed to parse atlas JSON: %s, errno: %d",
            jsonPath.c_str(), doc.GetParseError());
        return false;
    }

    int loadedCount = 0;
    for (auto it = doc.MemberBegin();
        it != doc.MemberEnd(); ++it) {
        TextureMetadata meta;
        meta.alias = it->name.GetString();
        const auto& desc = it->value;

        auto pathIt = desc.FindMember("path");
        if (pathIt != desc.MemberEnd() && pathIt->value.IsString()) {
            meta.path = pathIt->value.GetString();
        }

        auto widthIt = desc.FindMember("width");
        if (widthIt != desc.MemberEnd() && widthIt->value.IsInt()) {
            meta.width = widthIt->value.GetInt();
        }

        auto heightIt = desc.FindMember("height");
        if (heightIt != desc.MemberEnd() && heightIt->value.IsInt()) {
            meta.height = heightIt->value.GetInt();
        }

        auto formatIt = desc.FindMember("format");
        if (formatIt != desc.MemberEnd() && formatIt->value.IsInt()) {
            meta.channel = formatIt->value.GetInt();
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

    meta.textureId = CreatePlaceholderTexture(meta.width, meta.height, meta.channel);
    
    return meta.textureId;
}

void TextureRegistry::CreatePlaceholderTextures() {
    int createdCount = 0;
    for (auto& pair : m_textures) {
        TextureMetadata& meta = pair.second;

        if (meta.textureId != 0) {
            continue;
        }

        meta.textureId = CreatePlaceholderTexture(meta.width, meta.height, meta.channel);
        
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
    
    int channels = meta.channel;
    size_t expectedSize = meta.width * meta.height * channels;
    if (dataSize != expectedSize) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Data size mismatch for %s (expected %zu, got %zu)",
            alias.c_str(), expectedSize, dataSize);
        return false;
    }
    
    glBindTexture(GL_TEXTURE_2D, meta.textureId);
    
    GLenum format = GetGLFormat(meta.channel);
    glTexImage2D(GL_TEXTURE_2D, 0, format, meta.width, meta.height, 0, 
                 format, GL_UNSIGNED_BYTE, pixels);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    meta.isLoaded = true;

    // 检查该纹理是否是某些合成纹理的源，若是则尝试 re-bake
    auto srcIt = m_compositesBySource.find(alias);
    if (srcIt != m_compositesBySource.end()) {
        for (const auto& compAlias : srcIt->second) {
            BakeComposite(compAlias);
        }
    }

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
    m_compositesBySource.clear();
    DestroyBakePipeline();
}

GLenum TextureRegistry::GetGLFormat(int format) const {
    switch (format) {
        case 4: return GL_RGBA;
        case 3: return GL_RGB;
        case 1: return GL_R;  // Use luminance for single channel
        default: return GL_RGBA;
    }
}

GLuint TextureRegistry::CreatePlaceholderTexture(int width, int height, int format) {
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

// ============================================================
//  Composite Atlas
// ============================================================

bool TextureRegistry::LoadCompositeAtlas(const std::string& csvPath)
{
    std::ifstream file(csvPath);
    if (!file.is_open()) {
        // 文件不存在视为正常（没有合成纹理）
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
            "TextureRegistry: No composite atlas at '%s', skipping", csvPath.c_str());
        return true;
    }

    // 第一行为带类型的表头，忽略
    std::string line;
    if (!std::getline(file, line)) return true;

    // 解析各行
    // 格式: composite_alias, width, height, source_alias, anchor_x, anchor_y, pivot_x, pivot_y, scale
    int loadedLayers = 0;
    while (std::getline(file, line)) {
        // 去除末尾空白
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
            line.pop_back();
        if (line.empty()) continue;

        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            while (!tok.empty() && (tok.front() == ' ')) tok.erase(tok.begin());
            while (!tok.empty() && (tok.back()  == ' ')) tok.pop_back();
            tokens.push_back(tok);
        }
        if (tokens.size() < 9) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "TextureRegistry: Composite CSV bad row (need 9 cols): '%s'", line.c_str());
            continue;
        }

        const std::string& compAlias  = tokens[0];
        int   width      = std::stoi(tokens[1]);
        int   height     = std::stoi(tokens[2]);
        CompositeLayer layer;
        layer.sourceAlias = tokens[3];
        layer.anchorX     = std::stof(tokens[4]);
        layer.anchorY     = std::stof(tokens[5]);
        layer.pivotX      = std::stof(tokens[6]);
        layer.pivotY      = std::stof(tokens[7]);
        layer.scale       = std::stof(tokens[8]);

        // 注册或更新合成定义
        auto& def = m_textures[compAlias];
        if (def.alias.empty()) {
            def.alias  = compAlias;
            def.width  = width;
            def.height = height;
        }
        def.layers.push_back(layer);

        // 建立反向索引
        m_compositesBySource[layer.sourceAlias].push_back(compAlias);
        loadedLayers++;
    }
    return true;
}

bool TextureRegistry::loadCompositeAtlasJson(const std::string &jsonPath)
{
    std::ifstream ifs(jsonPath);
    if (!ifs.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Failed to open atlas JSON: %s", jsonPath.c_str());
        return false;
    }

    rapidjson::IStreamWrapper isw(ifs);

    rapidjson::Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TextureRegistry:  Failed to parse atlas JSON: %s, errno: %d",
            jsonPath.c_str(), doc.GetParseError());
        return false;
    }

    for (auto it = doc.MemberBegin();
        it != doc.MemberEnd(); ++it) {
        TextureMetadata meta;
        meta.alias = it->name.GetString();
        const auto& desc = it->value;

        int width = 0, height = 0;
        auto widthIt = desc.FindMember("width");
        if (widthIt != desc.MemberEnd() && widthIt->value.IsInt()) {
            meta.width = widthIt->value.GetInt();
        }
        auto heightIt = desc.FindMember("height");
        if (heightIt != desc.MemberEnd() && heightIt->value.IsInt()) {
            meta.height = heightIt->value.GetInt();
        }

        auto formatIt = desc.FindMember("format");
        if (formatIt != desc.MemberEnd() && formatIt->value.IsInt()) {
            meta.channel = formatIt->value.GetInt();
        }

        const auto& layers = desc["layers"];
        if (!layers.IsArray()) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "TextureRegistry: Composite JSON bad format (missing layers array): '%s'", meta.alias.c_str());
            continue;
        }

        for (const auto& layerDesc : layers.GetArray()) {
            CompositeLayer layer;
            layer.sourceAlias = layerDesc["source_alias"].GetString();
            layer.anchorX     = layerDesc["anchor_x"].GetFloat();
            layer.anchorY     = layerDesc["anchor_y"].GetFloat();
            layer.pivotX      = layerDesc["pivot_x"].GetFloat();
            layer.pivotY      = layerDesc["pivot_y"].GetFloat();
            layer.scale       = layerDesc["scale"].GetFloat();

            meta.layers.push_back(layer);
            m_compositesBySource[layer.sourceAlias].push_back(meta.alias);
        }
        m_textures[meta.alias] = meta;
    }

    return true;
}

// ============================================================
//  Bake pipeline - 内联 GLSL ES 300
// ============================================================

static const char* kBakeVert = R"(#version 300 es
precision mediump float;
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)";

static const char* kBakeFrag = R"(#version 300 es
precision mediump float;
in vec2 v_uv;
out vec4 fragColor;
uniform sampler2D u_tex;
void main() {
    fragColor = texture(u_tex, v_uv);
}
)";

static GLuint CompileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512]; glGetShaderInfoLog(s, 512, nullptr, buf);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "TextureRegistry bake shader error: %s", buf);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool TextureRegistry::InitBakePipeline()
{
    if (m_bakeProgram) return true;   // 已初始化

    GLuint vert = CompileShader(GL_VERTEX_SHADER,   kBakeVert);
    GLuint frag = CompileShader(GL_FRAGMENT_SHADER, kBakeFrag);
    if (!vert || !frag) { glDeleteShader(vert); glDeleteShader(frag); return false; }

    m_bakeProgram = glCreateProgram();
    glAttachShader(m_bakeProgram, vert);
    glAttachShader(m_bakeProgram, frag);
    glLinkProgram(m_bakeProgram);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint ok; glGetProgramiv(m_bakeProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512]; glGetProgramInfoLog(m_bakeProgram, 512, nullptr, buf);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "TextureRegistry bake program link error: %s", buf);
        glDeleteProgram(m_bakeProgram);
        m_bakeProgram = 0;
        return false;
    }

    // VAO + 动态 VBO（每层调用时填充）
    glGenVertexArrays(1, &m_bakeVAO);
    glGenBuffers(1, &m_bakeVBO);

    glBindVertexArray(m_bakeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_bakeVBO);
    // 每顶点: x, y, u, v  (4 × float，4 顶点 × triangle strip)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void TextureRegistry::DestroyBakePipeline()
{
    if (m_bakeVBO)     { glDeleteBuffers(1, &m_bakeVBO);       m_bakeVBO = 0; }
    if (m_bakeVAO)     { glDeleteVertexArrays(1, &m_bakeVAO);  m_bakeVAO = 0; }
    if (m_bakeProgram) { glDeleteProgram(m_bakeProgram);        m_bakeProgram = 0; }
}

// ============================================================
//  BakeComposite
// ============================================================

bool TextureRegistry::BakeComposite(const std::string& alias)
{
    auto defIt = m_textures.find(alias);
    if (defIt == m_textures.end()) return false;
    const auto& def = defIt->second;

    // 检查所有源纹理是否已载入
    for (const auto& layer : def.layers) {
        auto it = m_textures.find(layer.sourceAlias);
        if (it == m_textures.end() || it->second.textureId == 0) {
            SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
                "TextureRegistry: BakeComposite '%s' deferred (source '%s' not ready)",
                alias.c_str(), layer.sourceAlias.c_str());
            return false;
        }
    }

    if (!InitBakePipeline()) return false;

    // 确保输出纹理已分配
    auto& meta = m_textures[alias];
    if (meta.textureId == 0) {
        glGenTextures(1, &meta.textureId);
    }
    glBindTexture(GL_TEXTURE_2D, meta.textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, def.width, def.height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 创建 FBO
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, meta.textureId, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "TextureRegistry: FBO incomplete for composite '%s'", alias.c_str());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        return false;
    }

    // 保存渲染状态
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    glViewport(0, 0, def.width, def.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_bakeProgram);
    glUniform1i(glGetUniformLocation(m_bakeProgram, "u_tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_bakeVAO);

    // 逐层渲染
    for (const auto& layer : def.layers) {
        const TextureMetadata& src = m_textures[layer.sourceAlias];

        float srcW = src.width  * layer.scale;
        float srcH = src.height * layer.scale;

        // 图层左下角在 FBO 像素空间中的坐标（OpenGL：Y=0 在底部）
        float px = layer.anchorX * def.width  - layer.pivotX * srcW;
        float py = layer.anchorY * def.height - layer.pivotY * srcH;

        // 转换为 NDC [-1, 1]
        float ndcX0 = px          / def.width  * 2.0f - 1.0f;
        float ndcY0 = py          / def.height * 2.0f - 1.0f;
        float ndcX1 = (px + srcW) / def.width  * 2.0f - 1.0f;
        float ndcY1 = (py + srcH) / def.height * 2.0f - 1.0f;

        // Triangle strip: 左下, 右下, 左上, 右上
        // UV (0,0) = 视觉左下（stbi flip 后的 OpenGL 惯例）
        float quad[16] = {
            ndcX0, ndcY0,  0.0f, 0.0f,   // 左下
            ndcX1, ndcY0,  1.0f, 0.0f,   // 右下
            ndcX0, ndcY1,  0.0f, 1.0f,   // 左上
            ndcX1, ndcY1,  1.0f, 1.0f,   // 右上
        };

        glBindBuffer(GL_ARRAY_BUFFER, m_bakeVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindTexture(GL_TEXTURE_2D, src.textureId);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // 恢复状态
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

    meta.isLoaded = true;
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION,
        "TextureRegistry: Baked composite '%s' (%dx%d, %d layers)",
        alias.c_str(), def.width, def.height, (int)def.layers.size());
    return true;
}

int TextureRegistry::BakeCompositeTextures()
{
    int count = 0;
    for (const auto& pair : m_textures) {
        if (pair.second.layers.empty()) continue;
        if (BakeComposite(pair.first))
            ++count;
    }
    if (count > 0)
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
            "TextureRegistry: Baked %d composite texture(s)", count);
    return count;
}
