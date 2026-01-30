#include "Device.h"
#include "Window.h"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

Device::~Device()
{
}

class glDevice : public Device
{
public:
    glDevice(Window* window) : m_window(window) {}
	~glDevice() {

	}

	void initialize() override {
         auto* handle = reinterpret_cast<SDL_Window*>(m_window->getHandle());
#ifdef __EMSCRIPTEN__
        m_renderer = SDL_CreateRenderer(handle, -1, SDL_RENDERER_ACCELERATED);
        m_context = SDL_GL_GetCurrentContext();
#else
        m_context = SDL_GL_CreateContext(handle);
#endif
        if (!m_context) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create context: %s", SDL_GetError());
            return;
        }

#ifdef WIN32
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize GLAD");
            return;
        }
#endif

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForOpenGL(handle, m_context);
        ImGui_ImplOpenGL3_Init();
	}

	void cleanup() override {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        if (m_context) {
            SDL_GL_DeleteContext(m_context);
            m_context = nullptr;
        }
#ifdef __EMSCRIPTEN__
        if (m_renderer) {
            SDL_DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
#endif
	}

	void onResize() {
	}

private:
    Window* m_window = nullptr;
#ifdef __EMSCRIPTEN__
    SDL_Renderer* m_renderer = nullptr;
#endif
    SDL_GLContext m_context = nullptr;
};

Device* Device::createInstance(Window* window)
{
    return new glDevice(window);
}