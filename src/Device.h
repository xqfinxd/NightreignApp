#pragma once

#include "public.h"

class Window;
class Context;

class Device
{
public:
	static Device* createInstance();
	virtual ~Device();
	virtual void initialize(Window* window) = 0;
	virtual void cleanup() = 0;

	virtual void rebindWindow(Window* window) = 0;
};
