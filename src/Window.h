#pragma once

#include "public.h"
#include <glm/glm.hpp>

class Window
{
public:
	static Window* createInstance();
	virtual ~Window();
	virtual void initialize(int w, int h) = 0;
	virtual void cleanup() = 0;

	virtual void* getHandle() const {
		return nullptr;
	}

	virtual glm::ivec2 getSize() const = 0;
	virtual glm::ivec2 getCanvasSize() const = 0;

	virtual void swapBuffers() = 0;
};

