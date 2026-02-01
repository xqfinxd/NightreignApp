#pragma once

#include "public.h"
#include <string>
#include <cstddef>
#include <glm/glm.hpp>

class Window;
class Texture;
class Buffer;
class Shader;

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
	virtual	glm::vec2 getCanvasSize() const = 0;

	// Shader management
	virtual Shader* createShader(const std::string& vertPath, const std::string& fragPath) = 0;
	virtual void deleteShader(Shader* shader) = 0;

	// Texture management
	virtual Texture* createTexture(const std::string& path) = 0;
	virtual void deleteTexture(Texture* texture) = 0;

	// Buffer management
	virtual Buffer* createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data = nullptr) = 0;
	virtual void updateBuffer(Buffer* buffer, size_t offset, size_t size, const void* data) = 0;
	virtual void deleteBuffer(Buffer* buffer) = 0;

	// Vertex array management (for OpenGL)
	virtual void createVertexArray(uint32_t* vao, uint32_t* vbo) = 0;
	virtual void deleteVertexArray(uint32_t vao, uint32_t vbo) = 0;
};
