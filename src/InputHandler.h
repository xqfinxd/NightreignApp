#pragma once

#include "glm/glm.hpp"

#include <SDL.h>
#include <unordered_map>

struct InputState {
    enum Type {
        eNone,
        eSingleTap,
        eDoubleTap,
        eDragMove,
        eZoomLocal,
        eZoomCenter,
        eLongTouch,
    };

    Type type = eNone;
    glm::vec2 pos{};
    glm::vec2 delta{};
    float scale = 1.f;
};

class InputHandler {
public:
    struct TouchPoint {
        glm::vec2 startPos{};
        glm::vec2 curPos{};
        uint32_t startTime = 0;
        bool active = false;
    };
    
    InputState ProcessEvent(const SDL_Event& event, int w, int h) {
        InputState result = { InputState::eNone };
        if (!hasTouchDevice) {
            hasTouchDevice = SDL_GetNumTouchDevices() > 0;
        }

        switch (event.type) {
        case SDL_FINGERDOWN:
            HandleFingerDown(result, event.tfinger, w, h);
            break;

        case SDL_FINGERUP:
            HandleFingerUp(result, event.tfinger, w, h);
            break;

        case SDL_FINGERMOTION:
            HandleFingerMotion(result, event.tfinger, w, h);
            break;

        case SDL_MOUSEBUTTONDOWN:
            HandleMouseDown(result, event.button);
            break;

        case SDL_MOUSEBUTTONUP:
            HandleMouseUp(result, event.button);
            break;

        case SDL_MOUSEMOTION:
            HandleMouseMotion(result, event.motion);
            break;

        case SDL_MOUSEWHEEL:
            HandleMouseWheel(result, event.wheel);
            break;
        }

        return result;
    }

private:
    bool CheckInterval(uint32_t timestamp) const {
        return timestamp - lastTapTime <= DOUBLE_CLICK_INTERVAL;
    }

    bool CheckDistance(glm::vec2 v1, glm::vec2 v2) const {
        float dis = glm::distance(v1, v2);
        return dis <= DOUBLE_CLICK_DISTANCE;
    }

    void HandleFingerDown(InputState& result, const SDL_TouchFingerEvent& event, int w, int h) {
        if (activeTouches.size() == 2)
            return;
        TouchPoint& point = activeTouches[event.fingerId];
        point.startPos.x = point.curPos.x = event.x;
        point.startPos.y = point.curPos.y = event.y;
        point.startTime = event.timestamp;
        point.active = true;

        uint32_t delta = event.timestamp - lastTapTime;
        if (lastTapTime > 0
            && CheckInterval(event.timestamp)
            && CheckDistance(point.curPos, lastTapPos)) {
            
            result.type = InputState::eDoubleTap;
            result.pos.x = event.x * w;
            result.pos.y = event.y * h;
            result.scale = 2.f;

            lastTapTime = 0;
        }
        else {
            size_t count = activeTouches.size();
            if (count == 1) {
                lastTapTime = event.timestamp;
                lastTapPos = { event.x,event.y };
            }
            else if (count == 2) {
                // zoom start
                result.type = InputState::eZoomLocal;
                UpdateZoomData(result, w, h);
            }
        }
    }

    void HandleFingerUp(InputState& result, const SDL_TouchFingerEvent& event, int w, int h) {
        auto it = activeTouches.find(event.fingerId);
        if (it != activeTouches.end()) {
            TouchPoint& point = it->second;

            const size_t count = activeTouches.size();
            if (count == 1) {
                if (CheckDistance(point.startPos, point.curPos)) {
                    result.type = InputState::eSingleTap;
                    result.pos.x = event.x * w;
                    result.pos.y = event.y * h;
                }
            }
            else if (count == 2) {
                // zoom end
                ClearZoomData();
            }

            activeTouches.erase(it);
        }
    }

    void HandleFingerMotion(InputState& result, const SDL_TouchFingerEvent& event, int w, int h) {
        auto it = activeTouches.find(event.fingerId);
        if (it != activeTouches.end()) {
            TouchPoint& point = it->second;
            point.curPos = { event.x,event.y };

            const size_t count = activeTouches.size();
            if (count == 1) {
                result.type = InputState::eDragMove;
                result.pos.x = event.x * w;
                result.pos.y = event.y * h;
                result.delta.x = event.dx * w;
                result.delta.y = event.dy * h;
            }
            else if (count == 2) {
                result.type = InputState::eZoomLocal;
                UpdateZoomData(result, w, h);
            }
        }
    }

    void ClearZoomData() {
        initialDis = 0;
        curPos = { 0,0 };
        curZoom = 1;
    }

    void UpdateZoomData(InputState& result, int w, int h) {
        if (activeTouches.size() < 2) return;
        const auto& point1 = activeTouches[0];
        const auto& point2 = activeTouches[1];

        float dis = glm::distance(point1.curPos, point2.curPos);
        glm::vec2 center = (point1.curPos + point2.curPos) / 2.0f;

        result.pos.x = center.x * w;
        result.pos.y = center.y * h;
        if (initialDis == 0.0f) {
            initialDis = dis;
            curPos = center;
            curZoom = 1;
            result.scale = 1;
            result.delta = { 0,0 };
        }
        else {
            float scale = dis / initialDis;
            result.scale = scale / curZoom;
            glm::vec2 delta = center - curPos;
            result.delta.x = delta.x * w;
            result.delta.y = delta.y * h;
            curZoom = scale;
            curPos = center;
        }
    }

    void HandleMouseDown(InputState& result, const SDL_MouseButtonEvent& event) {
        if (hasTouchDevice)
            return;

        if (event.button == SDL_BUTTON_LEFT) {
            if (event.clicks == 1) {
                isDragging = true;
            }
            else if (event.clicks == 2) {
                result.type = InputState::eDoubleTap;
                result.pos = { event.x,event.y };
                result.scale = 2.f;
            }
        }
        else if (event.button == SDL_BUTTON_MIDDLE) {
            result.type = InputState::eLongTouch;
            result.pos = { event.x,event.y };
        }
    }

    void HandleMouseUp(InputState& result, const SDL_MouseButtonEvent& event) {
        if (hasTouchDevice)
            return;

        if (event.button == SDL_BUTTON_LEFT) {
            if (isDragging) {
                isDragging = false;
            }

            if(event.clicks == 1) {
                result.type = InputState::eSingleTap;
                result.pos = { event.x,event.y };
            }
        }
    }

    void HandleMouseMotion(InputState& result, const SDL_MouseMotionEvent& event) {
        if (hasTouchDevice)
            return;

        if (isDragging) {
            result.type = InputState::eDragMove;
            result.pos = { event.x,event.y };
            result.delta = { event.xrel,event.yrel };
        }
    }

    void HandleMouseWheel(InputState& result, const SDL_MouseWheelEvent& event) {
        if (hasTouchDevice)
            return;

        result.type = InputState::eZoomCenter;
        result.pos = { event.mouseX,event.mouseY };
        if (event.y > 0)
            result.scale = 2.f;
        else if (event.y < 0)
            result.scale = 0.5f;
    }

private:
    using TouchPoints = std::unordered_map<SDL_FingerID, TouchPoint>;
    
    bool hasTouchDevice = false;

    // mouse and finger
    bool isDragging = false;

    // only for finger
    TouchPoints activeTouches;
    // check double tap
    uint32_t lastTapTime = 0;
    glm::vec2 lastTapPos;

    float initialDis = 0;
    float curZoom = 1.0f;
    glm::vec2 curPos{ 0,0 };

    const uint32_t DOUBLE_CLICK_INTERVAL = 200;
    const float DOUBLE_CLICK_DISTANCE = 0.05f;
};