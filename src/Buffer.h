#pragma once
#include "public.h"
#include "Device.h"
#include <cstddef>
#include <vector>

class Buffer
{
public:
    Buffer(Device* device, BufferType type, BufferUsage usage);
    ~Buffer();

    // Initialize with data
    bool create(size_t size, const void* data = nullptr);
    
    // Update buffer data
    void update(size_t offset, size_t size, const void* data);
    void updateAll(const void* data);
    
    // Cleanup
    void destroy();

    // Getters
    uint32_t getID() const { return m_bufferId; }
    size_t getSize() const { return m_size; }
    BufferType getType() const { return m_type; }
    BufferUsage getUsage() const { return m_usage; }
    bool isValid() const { return m_bufferId != 0; }

    // Helper for uniform buffers - bind to a binding point
    void bindUniformBuffer(uint32_t bindingPoint) const;

private:
    Device* m_device = nullptr;
    uint32_t m_bufferId = 0;
    size_t m_size = 0;
    BufferType m_type;
    BufferUsage m_usage;
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

    if (!m_vertexBuffer) {
        m_vertexBuffer = new Buffer(m_device, BufferType::Vertex, usage);
    }

    m_vertexBuffer->create(vertices.size() * sizeof(T), vertices.data());
    m_vertexCount = vertices.size();
}

template<typename T>
void MeshBuffer::setIndexData(const std::vector<T>& indices, BufferUsage usage)
{
    if (indices.empty()) return;

    if (!m_indexBuffer) {
        m_indexBuffer = new Buffer(m_device, BufferType::Index, usage);
    }

    m_indexBuffer->create(indices.size() * sizeof(T), indices.data());
    m_indexCount = indices.size();
}
