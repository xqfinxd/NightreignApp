#pragma once
#include "public.h"

class Scene
{
public:
	Scene();
	~Scene();

	void initialize();
	void cleanup();

	void update(float deltaTime);
	void draw();

private:

};
