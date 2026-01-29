#include "Application.h"
#include "Window.h"
#include "Device.h"

#include <SDL_timer.h>
#include <SDL_log.h>
#include <SDL_events.h>
#include <imgui_impl_sdl2.h>

Application::Application()
{
    m_window = Window::createInstance();
    m_device = Device::createInstance();
}

Application::~Application()
{
    delete m_device;
    delete m_window;
}

void Application::start()
{
    if (m_running) return;

    initialize();

    m_running = true;
    m_last_frame_time = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    SDL_Log("Emscripten setup mainloop...");
    emscripten_set_main_loop_arg(runFrameWrapper, this, 0, 1);
#else
    while (m_running) {
        runFrame();
    }
    cleanup();
#endif
}

void Application::quit()
{
    m_running = false;
}

void Application::runFrame()
{
    if (!m_running) {
#ifdef __EMSCRIPTEN__
        SDL_Log("Emscripten terminate mainloop...");
        emscripten_cancel_main_loop();
        cleanup();
#endif
        return;
    }

    uint32_t curTime = SDL_GetTicks();
    float deltaTime = (curTime - m_last_frame_time) / 1000.0f;
    m_last_frame_time = curTime;

    processInput();
    update(SDL_min(deltaTime, 0.1f));
    render();

#ifndef __EMSCRIPTEN__
    if (m_fps > 0) {
        uint32_t frameTime = SDL_GetTicks() - curTime;
        uint32_t minFrameTime = 1000 / m_fps;
        if (frameTime < minFrameTime) {
            SDL_Delay(minFrameTime - frameTime);
        }
    }
#endif
}

void Application::runFrameWrapper(void* userData)
{
    if (auto* app = static_cast<Application*>(userData))
        app->runFrame();
}

void Application::initialize()
{
    m_window->initialize(1200, 900);
    m_device->initialize(m_window);
}

void Application::processInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            quit();
            break;
        }

        if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                m_device->rebindWindow(m_window);
            }
        }

        ImGui_ImplSDL2_ProcessEvent(&event);
        
    }
}

void Application::update(float deltaTime)
{
}

void Application::render()
{
}

void Application::cleanup()
{
    m_device->cleanup();
    m_window->cleanup();
}
