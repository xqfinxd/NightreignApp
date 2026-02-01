#pragma once
#include "public.h"
#include "Device.h"
#include <cstddef>
#include <vector>

class Buffer
{
public:
    Buffer();
    Buffer(uint32_t id, BufferType type, BufferUsage usage, size_t size);
    ~Buffer();

    // Getters
    uint32_t getID() const { return m_bufferId; }
    size_t getSize() const { return m_size; }
    BufferType getType() const { return m_type; }
    BufferUsage getUsage() const { return m_usage; }
    bool isValid() const { return m_bufferId != 0; }

    // Resource management
    void release();

private:
    uint32_t m_bufferId = 0;
    size_t m_size = 0;
    BufferType m_type = BufferType::Vertex;
    BufferUsage m_usage = BufferUsage::Static;
};

// Mesh buffer helper class
class MeshBuffer
{
public:
    MeshBuffer(Device* device);
    ~MeshBuffer();

    // Set vertex data
    template<typename T>
    void setVertexData(const std::vector<T>& vertices, BufferUsage usage = BufferUsage::Static);
    
    // Set index data
    template<typename T>
    void setIndexData(const std::vector<T>& indices, BufferUsage usage = BufferUsage::Static);

    // Cleanup
    void destroy();

    // Getters
    Buffer* getVertexBuffer() const { return m_vertexBuffer; }
    Buffer* getIndexBuffer() const { return m_indexBuffer; }
    size_t getVertexCount() const { return m_vertexCount; }
    size_t getIndexCount() const { return m_indexCount; }
    bool hasIndices() const { return m_indexBuffer != nullptr && m_indexBuffer->isValid(); }

private:
    Device* m_device = nullptr;
    Buffer* m_vertexBuffer = nullptr;
    Buffer* m_indexBuffer = nullptr;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
};

// Template implementations
template<typename T>
void MeshBuffer::setVertexData(const std::vector<T>& vertices, BufferUsage usage)
{
    if (vertices.empty()) return;

    // Delete old buffer if exists
    if (m_vertexBuffer && m_device) {
        m_device->deleteBuffer(m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }

    // Create new buffer
    if (m_device) {
        m_vertexBuffer = m_device->createBuffer(BufferType::Vertex, usage, vertices.size() * sizeof(T), vertices.data());
        m_vertexCount = vertices.size();
    }
}

template<typename T>
void MeshBuffer::setIndexData(const std::vector<T>& indices, BufferUsage usage)
{
    if (indices.empty()) return;

    // Delete old buffer if exists
    if (m_indexBuffer && m_device) {
        m_device->deleteBuffer(m_indexBuffer);
        m_indexBuffer = nullptr;
    }

    // Create new buffer
    if (m_device) {
        m_indexBuffer = m_device->createBuffer(BufferType::Index, usage, indices.size() * sizeof(T), indices.data());
        m_indexCount = indices.size();
    }
}
