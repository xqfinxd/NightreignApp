#include "Buffer.h"
#include <SDL_log.h>

// Buffer implementation
Buffer::Buffer()
    : m_bufferId(0)
    , m_size(0)
    , m_type(BufferType::Vertex)
    , m_usage(BufferUsage::Static)
{
}

Buffer::Buffer(uint32_t id, BufferType type, BufferUsage usage, size_t size)
    : m_bufferId(id)
    , m_size(size)
    , m_type(type)
    , m_usage(usage)
{
}

Buffer::~Buffer()
{
    // Note: Actual GPU resource cleanup should be handled by Device
    // This just releases the reference
}

void Buffer::release()
{
    m_bufferId = 0;
    m_size = 0;
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
    if (m_device) {
        if (m_vertexBuffer) {
            m_device->deleteBuffer(m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }
        
        if (m_indexBuffer) {
            m_device->deleteBuffer(m_indexBuffer);
            m_indexBuffer = nullptr;
        }
    }

    m_vertexCount = 0;
    m_indexCount = 0;
}
