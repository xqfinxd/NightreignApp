#include "Texture.h"

Texture::Texture()
	: m_id(0)
	, m_width(0)
	, m_height(0)
	, m_channel(0)
{
}

Texture::Texture(uint32_t id, int width, int height, int channel)
	: m_id(id)
	, m_width(width)
	, m_height(height)
	, m_channel(channel)
{
}

Texture::~Texture()
{
	// Note: Actual GPU resource cleanup should be handled by ResourceManager or Device
	// This just releases the reference
}

void Texture::release()
{
	m_id = 0;
	m_width = 0;
	m_height = 0;
	m_channel = 0;
}
