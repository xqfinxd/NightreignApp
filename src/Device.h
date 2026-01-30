#pragma once

#include "public.h"

class Window;
class Context;

class Device
{
public:
	static Device* createInstance(Window* window);
	virtual ~Device();
	virtual void initialize() = 0;
	virtual void cleanup() = 0;

	virtual void onResize() = 0;
};
