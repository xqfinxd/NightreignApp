#include "Texture.h"

Texture::Texture()
	: m_id(0)
	, m_width(0)
	, m_height(0)
	, m_format(TextureFormat::Unknown)
{
}

Texture::Texture(uint32_t id, int width, int height, TextureFormat format)
	: m_id(id)
	, m_width(width)
	, m_height(height)
	, m_format(format)
{
}

Texture::~Texture()
{
	// Note: Actual GPU resource cleanup should be handled by ResourceManager or Device
	// This just releases the reference
}

int Texture::getChannelCount() const
{
	switch (m_format) {
	case TextureFormat::R:
		return 1;
	case TextureFormat::RGB:
		return 3;
	case TextureFormat::RGBA:
		return 4;
	default:
		return 0;
	}
}

void Texture::release()
{
	m_id = 0;
	m_width = 0;
	m_height = 0;
	m_format = TextureFormat::Unknown;
}
