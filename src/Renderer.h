#pragma once
#include "public.h"

class Window;

class Renderer
{
public:
	Renderer(Window* window);
	~Renderer();

	void initialize();
	void cleanup();

	void beginFrame();
	void endFrame();

	void clear(const glm::vec4& color);
	void setViewport(int x, int y, int width, int height);

	void drawScene(class Scene* scene);

	Window* getWindow() const { return m_window; }

private:
	Window* m_window = nullptr;
};

