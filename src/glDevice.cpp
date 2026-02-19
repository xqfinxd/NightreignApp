#include "glDevice.h"
#include "Window.h"
#include "Texture.h"
#include "Buffer.h"
#include "Shader.h"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <stb_image.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <numeric>

glDevice::~glDevice() {

}

void glDevice::initialize() {
	auto* handle = reinterpret_cast<SDL_Window*>(m_window->getHandle());
#ifdef __EMSCRIPTEN__
	m_renderer = SDL_CreateRenderer(handle, -1, SDL_RENDERER_ACCELERATED);
	m_context = SDL_GL_GetCurrentContext();
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	m_context = SDL_GL_CreateContext(handle);
#endif
	if (!m_context) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create context: %s", SDL_GetError());
		return;
	}

#ifdef WIN32
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize GLAD");
		return;
	}
#endif

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	
	// Load Chinese font with limited character set
	ImGuiIO& io = ImGui::GetIO();
	
	// Read custom Chinese character set from chs.txt
	ImVector<ImWchar> ranges;
	std::string chsContent;
	chsContent.append(127, '\0');
    std::iota(chsContent.begin(), chsContent.end(), 1);
	
	std::ifstream chsFile("nightreign/assets/datas/chs.txt");
	if (chsFile.is_open()) {
		// Read all Chinese characters from file
		chsContent.append(std::istreambuf_iterator<char>(chsFile),
		                        std::istreambuf_iterator<char>());
		chsFile.close();
	} else {
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ImGui: Failed to open nightreign/assets/datas/chs.txt, using default Chinese font");
	}
	chsContent.push_back('\0');
	ImFontGlyphRangesBuilder myGlyph;
	myGlyph.AddText(chsContent.c_str());
	myGlyph.BuildRanges(&ranges);

	// Load font with custom or default ranges
	io.Fonts->AddFontFromFileTTF("nightreign/assets/fonts/simhei.ttf", 28.0f, nullptr, ranges.Data);
	io.Fonts->Build();
	
	ImGui_ImplSDL2_InitForOpenGL(handle, m_context);
	ImGui_ImplOpenGL3_Init();
	
	SDL_Log("ImGui initialized with Chinese font support");
}

void glDevice::cleanup() {
	// Clean up shaders
	for (Shader* shader : m_shaders) {
		if (shader && shader->isValid()) {
			uint32_t program = shader->getProgram();
			glDeleteProgram(program);
			shader->release();
			delete shader;
		}
	}
	m_shaders.clear();

	// Clean up textures
	for (Texture* texture : m_textures) {
		if (texture && texture->isValid()) {
			uint32_t textureId = texture->getId();
			glDeleteTextures(1, &textureId);
			texture->release();
			delete texture;
		}
	}
	m_textures.clear();

	// Clean up buffers
	for (Buffer* buffer : m_buffers) {
		if (buffer && buffer->isValid()) {
			uint32_t bufferId = buffer->getID();
			glDeleteBuffers(1, &bufferId);
			buffer->release();
			delete buffer;
		}
	}
	m_buffers.clear();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	if (m_context) {
		SDL_GL_DeleteContext(m_context);
		m_context = nullptr;
	}
#ifdef __EMSCRIPTEN__
	if (m_renderer) {
		SDL_DestroyRenderer(m_renderer);
		m_renderer = nullptr;
	}
#endif
}

void glDevice::onResize() {
	// OpenGL viewport will be updated by Application
}

glm::vec2 glDevice::getCanvasSize() const
{
    return m_window->getCanvasSize();
}

Shader* glDevice::createShader(const std::string& vertPath, const std::string& fragPath) {
	// Read shader files
	auto readFile = [](const std::string& path) -> std::string {
		std::ifstream file(path);
		if (!file.is_open()) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Device: Failed to open shader file: %s", path.c_str());
			return "";
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	};

	std::string vertSource = readFile(vertPath);
	std::string fragSource = readFile(fragPath);

	if (vertSource.empty() || fragSource.empty()) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Device: Failed to read shader files");
		return nullptr;
	}

	// Compile shaders
	auto compileShader = [](const std::string& source, uint32_t type) -> uint32_t {
		uint32_t shader = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		int success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Device: Shader compilation failed: %s", infoLog);
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	};

	uint32_t vertShader = compileShader(vertSource, GL_VERTEX_SHADER);
	if (vertShader == 0) return nullptr;

	uint32_t fragShader = compileShader(fragSource, GL_FRAGMENT_SHADER);
	if (fragShader == 0) {
		glDeleteShader(vertShader);
		return nullptr;
	}

	// Link program
	uint32_t program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, nullptr, infoLog);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Device: Shader linking failed: %s", infoLog);
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		glDeleteProgram(program);
		return nullptr;
	}

	// Clean up shader objects (no longer needed after linking)
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	Shader* shader = new Shader(program, vertPath, fragPath);
	m_shaders.push_back(shader);
	SDL_Log("Device: Created shader (%s, %s, Program ID: %u)",
		vertPath.c_str(), fragPath.c_str(), program);

	return shader;
}

void glDevice::deleteShader(Shader* shader) {
	if (shader && shader->isValid()) {
		uint32_t program = shader->getProgram();
		glDeleteProgram(program);
		auto it = std::find(m_shaders.begin(), m_shaders.end(), shader);
		if (it != m_shaders.end()) {
			m_shaders.erase(it);
		}
		shader->release();
		delete shader;
	}
}

Texture* glDevice::createTexture(const std::string& path) {
	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

	if (!data) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Device: Failed to load texture %s", path.c_str());
		return nullptr;
	}

	uint32_t textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Determine format
	GLenum format = GL_RGB;
	TextureFormat texFormat = TextureFormat::RGB;
	if (channels == 4) {
		format = GL_RGBA;
		texFormat = TextureFormat::RGBA;
	}
	else if (channels == 1) {
		format = GL_RED;
		texFormat = TextureFormat::R;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);

	Texture* texture = new Texture(textureId, width, height, texFormat);
	m_textures.push_back(texture);
	SDL_Log("Device: Created texture %s (%dx%d, %d channels, ID: %u)",
		path.c_str(), width, height, channels, textureId);

	return texture;
}

void glDevice::deleteTexture(Texture* texture) {
	if (texture && texture->isValid()) {
		uint32_t textureId = texture->getId();
		glDeleteTextures(1, &textureId);
		auto it = std::find(m_textures.begin(), m_textures.end(), texture);
		if (it != m_textures.end()) {
			m_textures.erase(it);
		}
		texture->release();
		delete texture;
	}
}

Buffer* glDevice::createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data) {
	GLuint bufferId;
	glGenBuffers(1, &bufferId);

	GLenum target = getGLBufferTarget(type);
	GLenum glUsage = getGLBufferUsage(usage);

	glBindBuffer(target, bufferId);
	glBufferData(target, size, data, glUsage);
	glBindBuffer(target, 0);

	Buffer* buffer = new Buffer(bufferId, type, usage, size);
	m_buffers.push_back(buffer);
	SDL_Log("Device: Created buffer (ID: %u, Type: %d, Usage: %d, Size: %zu bytes)",
		bufferId, static_cast<int>(type), static_cast<int>(usage), size);

	return buffer;
}

void glDevice::updateBuffer(Buffer* buffer, size_t offset, size_t size, const void* data) {
	if (!buffer || !buffer->isValid() || !data) return;

	if (offset + size > buffer->getSize()) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Device: Update range exceeds buffer size");
		return;
	}

	GLenum target = getGLBufferTarget(buffer->getType());
	glBindBuffer(target, buffer->getID());
	glBufferSubData(target, offset, size, data);
	glBindBuffer(target, 0);

	SDL_Log("Device: Updated buffer (ID: %u, Offset: %zu, Size: %zu bytes)",
		buffer->getID(), offset, size);
}

void glDevice::deleteBuffer(Buffer* buffer) {
	if (buffer && buffer->isValid()) {
		uint32_t bufferId = buffer->getID();
		glDeleteBuffers(1, &bufferId);
		auto it = std::find(m_buffers.begin(), m_buffers.end(), buffer);
		if (it != m_buffers.end()) {
			m_buffers.erase(it);
		}
		buffer->release();
		delete buffer;
	}
}

void glDevice::createVertexArray(uint32_t* vao, uint32_t* vbo) {
	glGenVertexArrays(1, vao);
	glGenBuffers(1, vbo);
	SDL_Log("Device: Created VAO: %u, VBO: %u", *vao, *vbo);
}

void glDevice::deleteVertexArray(uint32_t vao, uint32_t vbo)
{
	if (vbo) glDeleteBuffers(1, &vbo);
	if (vao) glDeleteVertexArrays(1, &vao);
}

GLenum glDevice::getGLBufferTarget(BufferType type) {
	switch (type) {
	case BufferType::Vertex: return GL_ARRAY_BUFFER;
	case BufferType::Index: return GL_ELEMENT_ARRAY_BUFFER;
	case BufferType::Uniform: return GL_UNIFORM_BUFFER;
	default: return GL_ARRAY_BUFFER;
	}
}

GLenum glDevice::getGLBufferUsage(BufferUsage usage) {
	switch (usage) {
	case BufferUsage::Static: return GL_STATIC_DRAW;
	case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
	case BufferUsage::Stream: return GL_STREAM_DRAW;
	default: return GL_STATIC_DRAW;
	}
}
