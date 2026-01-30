#include "Application.h"
#include "Window.h"
#include "Device.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "systems/CameraSystem.h"
#include "ECS.h"

#include <SDL_timer.h>
#include <SDL_log.h>
#include <SDL_events.h>
#include <imgui_impl_sdl2.h>

Application::Application()
{
    SDL_Log("Application: Creating application...");
    m_window = Window::createInstance();
    m_device = Device::createInstance(m_window);
    m_renderer = new Renderer(m_window);
    m_scene_mgr = new SceneManager();
    m_resource_mgr = new ResourceManager(m_device);
    SDL_Log("Application: Application created successfully");
}

Application::~Application()
{
    SDL_Log("Application: Destroying application...");
    delete m_scene_mgr;
    delete m_resource_mgr;
    delete m_renderer;
    delete m_device;
    delete m_window;
    SDL_Log("Application: Application destroyed");
}

void Application::start()
{
    if (m_running) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Application: Already running");
        return;
    }

    SDL_Log("Application: Starting application...");
    initialize();

    m_running = true;
    m_last_frame_time = SDL_GetTicks();
    SDL_Log("Application: Entering main loop (FPS: %d)", m_fps);

#ifdef __EMSCRIPTEN__
    SDL_Log("Emscripten setup mainloop...");
    emscripten_set_main_loop_arg(runFrameWrapper, this, 0, 1);
#else
    while (m_running) {
        runFrame();
    }
    SDL_Log("Application: Exited main loop");
    cleanup();
#endif
}

void Application::quit()
{
    SDL_Log("Application: Quit requested");
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
    SDL_Log("Application: Initializing subsystems...");
    m_window->initialize(1200, 900);
    m_device->initialize();
    m_renderer->initialize();
    m_resource_mgr->initialize();
    m_scene_mgr->initialize();

    // Add default scene
    m_scene_mgr->addScene("main", new Scene());

    // Set initial viewport
    auto size = m_window->getCanvasSize();
    m_renderer->setViewport(0, 0, size.x, size.y);
    SDL_Log("Application: Initialization complete (Viewport: %dx%d)", size.x, size.y);
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
                m_device->onResize();
                auto size = m_window->getCanvasSize();
                m_renderer->setViewport(0, 0, size.x, size.y);
                SDL_Log("Application: Window resized to %dx%d", size.x, size.y);
            }
        }

        ImGui_ImplSDL2_ProcessEvent(&event);
        
    }
}

void Application::update(float deltaTime)
{
    m_scene_mgr->update(deltaTime);
}

void Application::render()
{
    m_renderer->beginFrame();
    m_scene_mgr->draw(m_renderer);
    m_renderer->endFrame();
}

void Application::cleanup()
{
    SDL_Log("Application: Cleaning up subsystems...");
    m_scene_mgr->cleanup();
    m_resource_mgr->cleanup();
    m_renderer->cleanup();
    m_device->cleanup();
    m_window->cleanup();
    SDL_Log("Application: Cleanup complete");
}
