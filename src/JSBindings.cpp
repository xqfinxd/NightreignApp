#include "Application.h"
#include "SceneManager.h"
#include "Scene.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/em_js.h>
#include <string>

// Global application instance pointer
static Application* g_app = nullptr;

void setApplicationInstance(Application* app) {
    g_app = app;
}

// C++ function to call JavaScript showToast
// EM_JS allows calling JavaScript code from C++
EM_JS(void, jsShowToast, (const char* message, int duration), {
    if (typeof Module !== 'undefined' && Module.showToast) {
        Module.showToast(UTF8ToString(message), duration);
    } else {
        console.warn('Module.showToast is not available');
    }
});

// Wrapper function with std::string support
void showToast(const std::string& message, int duration) {
    jsShowToast(message.c_str(), duration);
}

// C++ function to call JavaScript setInfoPanelContent
EM_JS(void, jsSetInfoPanelContent, (const char* htmlContent), {
    if (typeof Module !== 'undefined' && Module.setInfoPanelContent) {
        Module.setInfoPanelContent(UTF8ToString(htmlContent));
    } else {
        console.warn('Module.setInfoPanelContent is not available');
    }
});

// Wrapper function with std::string support
void setInfoPanelContent(const std::string& htmlContent) {
    jsSetInfoPanelContent(htmlContent.c_str());
}

void jsFilterMap(int mapIndex) {
    if (!g_app) return;
    
    auto* sceneMgr = g_app->getSceneManager();
    if (!sceneMgr) return;
    
    auto* scene = sceneMgr->getActiveScene();
    if (!scene) return;
    
    scene->filterMap(mapIndex);
}

void jsFilterNightlord(int nightlordIndex) {
    if (!g_app) return;
    
    auto* sceneMgr = g_app->getSceneManager();
    if (!sceneMgr) return;
    
    auto* scene = sceneMgr->getActiveScene();
    if (!scene) return;
    
    scene->filterNightlord(nightlordIndex);
}

void jsToggleB1Overlay() {
    if (!g_app) return;
    
    auto* sceneMgr = g_app->getSceneManager();
    if (!sceneMgr) return;
    
    auto* scene = sceneMgr->getActiveScene();
    if (!scene) return;
    
    bool newState = !scene->getEnableB1Overlay();
    scene->setEnableB1Overlay(newState);
    
    showToast(newState ? "地下显示: 开" : "地下显示: 关");
}

EMSCRIPTEN_BINDINGS(nightreign_bindings) {
    emscripten::function("setApplicationInstance", &setApplicationInstance, emscripten::allow_raw_pointers());
    emscripten::function("filterMap", &jsFilterMap);
    emscripten::function("filterNightlord", &jsFilterNightlord);
    emscripten::function("toggleB1Overlay", &jsToggleB1Overlay);
}
#else
void showToast(const std::string& message, int duration) {
}
void setInfoPanelContent(const std::string& htmlContent) {
}
#endif // __EMSCRIPTEN__
