#pragma once

#include "public.h"
#include <string>
#include <cstddef>
#include <any>

class Window;

enum class BufferType {
	Vertex,
	Index,
	Uniform
};

enum class BufferUsage {
	Static,   // Data rarely changes
	Dynamic,  // Data changes frequently
	Stream    // Data changes every frame
};

class Device
{
public:
	virtual ~Device() {}
	virtual void initialize() = 0;
	virtual void cleanup() = 0;

	virtual void onResize() = 0;

	// Texture management
	virtual uint32_t createTexture(const std::string& path) = 0;
	virtual void deleteTexture(uint32_t textureId) = 0;

	// Buffer management
	virtual uint32_t createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data = nullptr) = 0;
	virtual void updateBuffer(uint32_t bufferId, BufferType type, size_t offset, size_t size, const void* data) = 0;
	virtual void deleteBuffer(uint32_t bufferId) = 0;

	// Vertex array management (for OpenGL)
	virtual void createVertexArray(uint32_t* vao, uint32_t* vbo) = 0;
	virtual void deleteVertexArray(uint32_t vao, uint32_t vbo) = 0;
};
