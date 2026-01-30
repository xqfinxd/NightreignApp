#include "Buffer.h"
#include <SDL_log.h>

// Buffer implementation
Buffer::Buffer(Device* device, BufferType type, BufferUsage usage)
    : m_device(device)
    , m_type(type)
    , m_usage(usage)
{
}

Buffer::~Buffer()
{
    destroy();
}

bool Buffer::create(size_t size, const void* data)
{
    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Buffer: No device available");
        return false;
    }

    if (size == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Buffer: Cannot create buffer with size 0");
        return false;
    }

    // Destroy existing buffer if any
    destroy();

    m_bufferId = m_device->createBuffer(m_type, m_usage, size, data);
    m_size = size;

    if (m_bufferId == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Buffer: Failed to create buffer");
        return false;
    }

    return true;
}

void Buffer::update(size_t offset, size_t size, const void* data)
{
    if (!m_device || m_bufferId == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Buffer: Cannot update invalid buffer");
        return;
    }

    if (offset + size > m_size) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Buffer: Update range exceeds buffer size");
        return;
    }

    m_device->updateBuffer(m_bufferId, m_type, offset, size, data);
}

void Buffer::updateAll(const void* data)
{
    update(0, m_size, data);
}

void Buffer::destroy()
{
    if (m_device && m_bufferId != 0) {
        m_device->deleteBuffer(m_bufferId);
        m_bufferId = 0;
        m_size = 0;
    }
}

void Buffer::bindUniformBuffer(uint32_t bindingPoint) const
{
    if (m_bufferId == 0 || m_type != BufferType::Uniform) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Buffer: Cannot bind non-uniform buffer or invalid buffer");
        return;
    }

#ifndef __EMSCRIPTEN__
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_bufferId);
#else
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Buffer: Uniform buffer binding not supported on WebGL");
#endif
}

// MeshBuffer implementation
MeshBuffer::MeshBuffer(Device* device)
    : m_device(device)
{
}

MeshBuffer::~MeshBuffer()
{
    destroy();
}

void MeshBuffer::destroy()
{
    if (m_vertexBuffer) {
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;
    }
    
    if (m_indexBuffer) {
        delete m_indexBuffer;
        m_indexBuffer = nullptr;
    }

    m_vertexCount = 0;
    m_indexCount = 0;
}
