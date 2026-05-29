////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Input Injection Implementation
////////////////////////////////////////////////////////////////////////////////

#include "input_inject.h"
#include "engine/input.h"
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_events.h>
#include <algorithm>
#include <iostream>

namespace vt_mod {

namespace {

std::string ToLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

} // anonymous namespace

InputInjector::InputInjector() : _mutex(SDL_CreateMutex()) {}

InputInjector::~InputInjector() {
    if (_mutex) SDL_DestroyMutex(_mutex);
}

InputInjector* InputInjector::SingletonGet() {
    static InputInjector inst;
    return &inst;
}

SDL_Keycode InputInjector::_KeyNameToSDL(const std::string& key) {
    std::string k = ToLower(key);
    if (k == "up")     return SDLK_UP;
    if (k == "down")   return SDLK_DOWN;
    if (k == "left")   return SDLK_LEFT;
    if (k == "right")  return SDLK_RIGHT;
    if (k == "confirm") return SDLK_RETURN;     // Enter
    if (k == "cancel") return SDLK_ESCAPE;
    if (k == "menu")   return SDLK_ESCAPE;
    if (k == "pause")  return SDLK_p;
    if (k == "minimap") return SDLK_TAB;
    if (k == "escape" || k == "esc") return SDLK_ESCAPE;
    if (k == "return" || k == "enter") return SDLK_RETURN;
    return SDLK_UNKNOWN;
}

void InputInjector::_InjectKeyDown(SDL_Keycode key) {
    SDL_Event ev;
    ev.type = SDL_KEYDOWN;
    ev.key.state = SDL_PRESSED;
    ev.key.repeat = 0;
    ev.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;
    ev.key.keysym.sym = key;
    ev.key.keysym.mod = KMOD_NONE;
    SDL_PushEvent(&ev);
}

void InputInjector::_InjectKeyUp(SDL_Keycode key) {
    SDL_Event ev;
    ev.type = SDL_KEYUP;
    ev.key.state = SDL_RELEASED;
    ev.key.repeat = 0;
    ev.key.keysym.scancode = SDL_SCANCODE_UNKNOWN;
    ev.key.keysym.sym = key;
    ev.key.keysym.mod = KMOD_NONE;
    SDL_PushEvent(&ev);
}

void InputInjector::QueueAction(const std::string& key, int duration_ms) {
    SDL_LockMutex(_mutex);
    KeyAction action;
    action.key = ToLower(key);
    action.duration_ms = duration_ms;
    action.pressed = false;
    action.press_time = 0;
    _actions.push_back(action);
    SDL_UnlockMutex(_mutex);
}

void InputInjector::Update() {
    SDL_LockMutex(_mutex);
    if (_actions.empty()) {
        SDL_UnlockMutex(_mutex);
        return;
    }

    Uint32 now = SDL_GetTicks();
    std::vector<KeyAction> remaining;

    for (auto& action : _actions) {
        SDL_Keycode keycode = _KeyNameToSDL(action.key);
        if (keycode == SDLK_UNKNOWN) continue;

        if (!action.pressed) {
            // Key not yet pressed — press it now
            _InjectKeyDown(keycode);
            action.pressed = true;
            action.press_time = now;
            remaining.push_back(action);
        } else {
            // Key is pressed — check if duration elapsed
            if (static_cast<int>(now - action.press_time) >= action.duration_ms) {
                _InjectKeyUp(keycode);
                // don't keep — action complete
            } else {
                remaining.push_back(action);
            }
        }
    }

    _actions.swap(remaining);
    SDL_UnlockMutex(_mutex);
}

void InputInjector::Clear() {
    SDL_LockMutex(_mutex);
    _actions.clear();
    SDL_UnlockMutex(_mutex);
}

bool InputInjector::IsActive() const {
    SDL_LockMutex(_mutex);
    bool active = !_actions.empty();
    SDL_UnlockMutex(_mutex);
    return active;
}

} // namespace vt_mod