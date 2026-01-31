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

	void onResize();

	uint32_t createTexture(const std::string& path) override;

	void deleteTexture(uint32_t textureId) override;

	uint32_t createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data) override;

	void updateBuffer(uint32_t bufferId, BufferType type, size_t offset, size_t size, const void* data) override;

	void deleteBuffer(uint32_t bufferId) override;

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
	std::vector<uint32_t> m_textures;
	std::vector<uint32_t> m_buffers;
};
