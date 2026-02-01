#pragma once

#include "public.h"
#include <string>

enum class TextureFormat {
	Unknown = 0,
	R,       // Single channel (grayscale)
	RGB,     // 3 channels
	RGBA     // 4 channels with alpha
};

class Texture
{
public:
	Texture();
	Texture(uint32_t id, int width, int height, TextureFormat format);
	~Texture();

	// Getters
	uint32_t getId() const { return m_id; }
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	TextureFormat getFormat() const { return m_format; }
	int getChannelCount() const;
	
	// Validation
	bool isValid() const { return m_id != 0; }

	// Resource management
	void release();

private:
	uint32_t m_id = 0;
	int m_width = 0;
	int m_height = 0;
	TextureFormat m_format = TextureFormat::Unknown;
};
