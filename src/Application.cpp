#include "Application.h"
#include "Window.h"
#include "Device.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "sdlWindow.h"
#include "glDevice.h"

#include <SDL_timer.h>
#include <SDL_log.h>
#include <SDL_events.h>
#include <imgui_impl_sdl2.h>

Application::Application()
{
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Creating application...");
    m_window = new sdlWindow();
    m_device = new glDevice(m_window);
    m_resource_mgr = new ResourceManager(m_device);
    m_renderer = new Renderer(m_device, m_resource_mgr);
    m_scene_mgr = new SceneManager();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Application created successfully");
}

Application::~Application()
{
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Destroying application...");
    delete m_scene_mgr;
    delete m_resource_mgr;
    delete m_renderer;
    delete m_device;
    delete m_window;
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Application destroyed");
}

void Application::start()
{
    if (m_running) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Application: Already running");
        return;
    }

    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Starting application...");
    initialize();

    m_running = true;
    m_last_frame_time = SDL_GetTicks();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Entering main loop (FPS: %d)", m_fps);

#ifdef __EMSCRIPTEN__
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Emscripten setup mainloop...");
    emscripten_set_main_loop_arg(runFrameWrapper, this, 0, 1);
#else
    while (m_running) {
        runFrame();
    }
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Exited main loop");
    cleanup();
#endif
}

void Application::quit()
{
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Quit requested");
    m_running = false;
}

void Application::runFrame()
{
    if (!m_running) {
#ifdef __EMSCRIPTEN__
        SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Emscripten terminate mainloop...");
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
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Initializing subsystems...");
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
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Initialization complete (Viewport: %dx%d)", size.x, size.y);
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
                SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Window resized to %dx%d", size.x, size.y);
            }
        }

		// Handle mouse clicks for spots
		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
		{
			// Only handle click if not over ImGui window
			if (!ImGui::GetIO().WantCaptureMouse)
			{
				auto size = m_window->getCanvasSize();
				if (auto activeScene = m_scene_mgr->getActiveScene())
				{
					activeScene->onMouseClick(event.button.x, event.button.y, size.x, size.y);
				}
			}
		}
		
		// Handle right click for context menu
		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
		{
			// Only handle click if not over ImGui window
			if (!ImGui::GetIO().WantCaptureMouse)
			{
				auto size = m_window->getCanvasSize();
				if (auto activeScene = m_scene_mgr->getActiveScene())
				{
					activeScene->onMouseRightClick(event.button.x, event.button.y, size.x, size.y);
				}
			}
		}

		// Handle mouse button press/release for dragging
		if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
		{
			if (!ImGui::GetIO().WantCaptureMouse)
			{
				if (auto activeScene = m_scene_mgr->getActiveScene())
				{
					bool pressed = (event.type == SDL_MOUSEBUTTONDOWN);
					activeScene->onMouseButton(event.button.button, pressed);
				}
			}
		}

		// Handle mouse motion for dragging
		if (event.type == SDL_MOUSEMOTION)
		{
			if (!ImGui::GetIO().WantCaptureMouse)
			{
				auto size = m_window->getCanvasSize();
				if (auto activeScene = m_scene_mgr->getActiveScene())
				{
					activeScene->onMouseMove(event.motion.x, event.motion.y, size.x, size.y);
				}
			}
		}

		// Handle mouse wheel for zooming
		if (event.type == SDL_MOUSEWHEEL)
		{
			if (!ImGui::GetIO().WantCaptureMouse)
			{
				if (auto activeScene = m_scene_mgr->getActiveScene())
				{
					activeScene->onMouseWheel(event.wheel.y * 1.0f);
				}
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
    if (auto activeScene = m_scene_mgr->getActiveScene())
    {
        m_renderer->drawScene(activeScene);
        
        activeScene->drawUI();
    }
    m_renderer->endFrame();
    m_window->swapBuffers();
}

void Application::cleanup()
{
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Cleaning up subsystems...");
    m_scene_mgr->cleanup();
    m_resource_mgr->cleanup();
    m_renderer->cleanup();
    m_device->cleanup();
    m_window->cleanup();
    SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "Application: Cleanup complete");
}
