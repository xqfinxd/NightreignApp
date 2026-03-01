#pragma once

#include "public.h"
#include <string>

class Texture
{
public:
	Texture();
	Texture(uint32_t id, int width, int height, int channel);
	~Texture();

	// Getters
	uint32_t getId() const { return m_id; }
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
	int getChannel() const { return m_channel; }
	
	// Validation
	bool isValid() const { return m_id != 0; }

	// Resource management
	void release();

private:
	uint32_t m_id = 0;
	int m_width = 0;
	int m_height = 0;
	int m_channel = 0;
};
