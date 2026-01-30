#include "Device.h"
#include "Window.h"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <vector>
#include <stb_image.h>

Device::~Device()
{
}

class glDevice : public Device
{
public:
    glDevice(Window* window) : m_window(window) {}
	~glDevice() {

	}

	void initialize() override {
         auto* handle = reinterpret_cast<SDL_Window*>(m_window->getHandle());
#ifdef __EMSCRIPTEN__
        m_renderer = SDL_CreateRenderer(handle, -1, SDL_RENDERER_ACCELERATED);
        m_context = SDL_GL_GetCurrentContext();
#else
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
        ImGui_ImplSDL2_InitForOpenGL(handle, m_context);
        ImGui_ImplOpenGL3_Init();
	}

	void cleanup() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

		// Cleanup all tracked textures
		for (uint32_t textureId : m_textures) {
			glDeleteTextures(1, &textureId);
		}
		m_textures.clear();

		// Cleanup all tracked buffers
		for (uint32_t bufferId : m_buffers) {
			glDeleteBuffers(1, &bufferId);
		}
		m_buffers.clear();

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

	void onResize() {
		// OpenGL viewport will be updated by Application
	}

	uint32_t createTexture(const std::string& path) override {
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

		if (!data) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Device: Failed to load texture %s", path.c_str());
			return 0;
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
		if (channels == 4) {
			format = GL_RGBA;
		} else if (channels == 1) {
			format = GL_RED;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(data);

		m_textures.push_back(textureId);
		SDL_Log("Device: Created texture %s (%dx%d, %d channels, ID: %u)", 
				path.c_str(), width, height, channels, textureId);

		return textureId;
	}

	void deleteTexture(uint32_t textureId) override {
		if (textureId > 0) {
			glDeleteTextures(1, &textureId);
			auto it = std::find(m_textures.begin(), m_textures.end(), textureId);
			if (it != m_textures.end()) {
				m_textures.erase(it);
			}
		}
	}

	uint32_t createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data) override {
		GLuint bufferId;
		glGenBuffers(1, &bufferId);

		GLenum target = getGLBufferTarget(type);
		GLenum glUsage = getGLBufferUsage(usage);

		glBindBuffer(target, bufferId);
		glBufferData(target, size, data, glUsage);
		glBindBuffer(target, 0);

		m_buffers.push_back(bufferId);
		SDL_Log("Device: Created buffer (ID: %u, Type: %d, Usage: %d, Size: %zu bytes)", 
				bufferId, static_cast<int>(type), static_cast<int>(usage), size);

		return bufferId;
	}

	void updateBuffer(uint32_t bufferId, BufferType type, size_t offset, size_t size, const void* data) override {
		if (bufferId == 0 || !data) return;

		GLenum target = getGLBufferTarget(type);
		glBindBuffer(target, bufferId);
		glBufferSubData(target, offset, size, data);
		glBindBuffer(target, 0);

		SDL_Log("Device: Updated buffer (ID: %u, Offset: %zu, Size: %zu bytes)", 
				bufferId, offset, size);
	}

	void deleteBuffer(uint32_t bufferId) override {
		if (bufferId > 0) {
			glDeleteBuffers(1, &bufferId);
			auto it = std::find(m_buffers.begin(), m_buffers.end(), bufferId);
			if (it != m_buffers.end()) {
				m_buffers.erase(it);
			}
		}
	}

	void createVertexArray(uint32_t* vao, uint32_t* vbo) override {
		glGenVertexArrays(1, vao);
		glGenBuffers(1, vbo);
		SDL_Log("Device: Created VAO: %u, VBO: %u", *vao, *vbo);
	}

	void deleteVertexArray(uint32_t vao, uint32_t vbo) override {
		if (vbo) glDeleteBuffers(1, &vbo);
		if (vao) glDeleteVertexArrays(1, &vao);
	}

private:
	GLenum getGLBufferTarget(BufferType type) {
		switch (type) {
			case BufferType::Vertex: return GL_ARRAY_BUFFER;
			case BufferType::Index: return GL_ELEMENT_ARRAY_BUFFER;
			case BufferType::Uniform: return GL_UNIFORM_BUFFER;
			default: return GL_ARRAY_BUFFER;
		}
	}

	GLenum getGLBufferUsage(BufferUsage usage) {
		switch (usage) {
			case BufferUsage::Static: return GL_STATIC_DRAW;
			case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
			case BufferUsage::Stream: return GL_STREAM_DRAW;
			default: return GL_STATIC_DRAW;
		}
	}

private:
    Window* m_window = nullptr;
#ifdef __EMSCRIPTEN__
    SDL_Renderer* m_renderer = nullptr;
#endif
    SDL_GLContext m_context = nullptr;
    std::vector<uint32_t> m_textures;
    std::vector<uint32_t> m_buffers;
};

Device* Device::createInstance(Window* window)
{
    return new glDevice(window);
}