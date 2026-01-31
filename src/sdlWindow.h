#pragma once

#include "Window.h"
#include <SDL.h>

class sdlWindow : public Window
{
public:
    ~sdlWindow();

    void initialize(int w, int h) override;

    void cleanup() override;

    virtual void* getHandle() const;

    glm::ivec2 getSize() const override;

    glm::ivec2 getCanvasSize() const override;

    void swapBuffers() override;

private:
    SDL_Window* m_handle = nullptr;
};
