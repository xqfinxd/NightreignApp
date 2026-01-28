#include <SDL.h>
#include <imgui.h>

int main(int argc, char** argv)
{
    SDL_Log("Hello World!");
    auto& style = ImGui::GetStyle();
    SDL_Log("ImGui: %f", style.Alpha);
    return 0;
}