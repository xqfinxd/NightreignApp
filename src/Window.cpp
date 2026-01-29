#include "Window.h"
#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

Window::~Window()
{
}

class sdlWindow : public Window
{
public:
    ~sdlWindow() {
    }

	void initialize(int w, int h) override {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL: %s", SDL_GetError());
            return;
        }

        int flags = SDL_WINDOW_SHOWN;
#ifndef __EMSCRIPTEN__
        if (w <= 0 || h <= 0)
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        flags |= SDL_WINDOW_OPENGL;
#else
        flags |= SDL_WINDOW_RESIZABLE;

        double cssWidth, cssHeight;
        emscripten_get_element_css_size("canvas", &cssWidth, &cssHeight);
        // emscripten_set_canvas_element_size("canvas", (int)width, (int)height);
        
        w = static_cast<int>(cssWidth);
        h = static_cast<int>(cssHeight);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Window size: %d, %d", w, h);
#endif
        m_handle = SDL_CreateWindow("Nightreign App",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            w, h, flags);
        if (!m_handle) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window: %s", SDL_GetError());
            return;
        }
	}

	void cleanup() override {
        if (m_handle) {
            SDL_DestroyWindow(m_handle);
            m_handle = nullptr;
        }
        SDL_Quit();
	}

	virtual void* getHandle() const {
		return m_handle;
	}

	glm::ivec2 getSize() const override {
        glm::ivec2 size;
		SDL_GetWindowSize(m_handle, &size.x, &size.y);
        return size;
	}

	glm::ivec2 getCanvasSize() const override {
        glm::ivec2 size;
        SDL_GetWindowSizeInPixels(m_handle, &size.x, &size.y);
        return size;
	}

private:
	SDL_Window* m_handle = nullptr;
};

Window* Window::createInstance()
{
    return new sdlWindow;
}