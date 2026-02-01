#pragma once

#include "Device.h"
#include <SDL_video.h>
#include <vector>

class glDevice : public Device
{
public:
	glDevice(Window* window) : m_window(window) {}
	~glDevice();

	void initialize() override;

	void cleanup() override;

	void onResize() override;

	glm::vec2 getCanvasSize() const override;

	Shader* createShader(const std::string& vertPath, const std::string& fragPath) override;

	void deleteShader(Shader* shader) override;

	Texture* createTexture(const std::string& path) override;

	void deleteTexture(Texture* texture) override;

	Buffer* createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data) override;

	void updateBuffer(Buffer* buffer, size_t offset, size_t size, const void* data) override;

	void deleteBuffer(Buffer* buffer) override;

	void createVertexArray(uint32_t* vao, uint32_t* vbo) override;

	void deleteVertexArray(uint32_t vao, uint32_t vbo) override {
		if (vbo) glDeleteBuffers(1, &vbo);
		if (vao) glDeleteVertexArrays(1, &vao);
	}

private:
	GLenum getGLBufferTarget(BufferType type);

	GLenum getGLBufferUsage(BufferUsage usage);

private:
	Window* m_window = nullptr;
#ifdef __EMSCRIPTEN__
	SDL_Renderer* m_renderer = nullptr;
#endif
	SDL_GLContext m_context = nullptr;
	std::vector<Shader*> m_shaders;
	std::vector<Texture*> m_textures;
	std::vector<Buffer*> m_buffers;
};
