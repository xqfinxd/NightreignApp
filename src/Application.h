#pragma once
#include "public.h"

class Window;
class Device;
class Renderer;
class InputManager;
class ResourceManager;
class ModuleManager;

class Application {
public:
    Application();
    ~Application();

    void start();
    void quit();

    void setFPS(int fps) { m_fps = fps; }
    int getFPS() const { return m_fps; }

private:
    void runFrame();
    static void runFrameWrapper(void*);

    void initialize();
    void processInput();
    void update(float deltaTime);
    void render();
    void cleanup();

private:
    bool m_running = false;
    int m_fps = 30;
    uint32_t m_last_frame_time = 0;

private:
    Window* m_window = nullptr;
    Device* m_device = nullptr;
    Renderer* m_renderer = nullptr;
    InputManager* m_input_mgr = nullptr;
    ResourceManager* m_resource_mgr = nullptr;
    ModuleManager* m_module_mgr = nullptr;
};
