////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Input Injection Implementation
////////////////////////////////////////////////////////////////////////////////

#include "input_inject.h"
#include "engine/input.h"
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_events.h>
#include <algorithm>

namespace vt_mod {

namespace {

std::string ToLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

// Named action sequences (predefined multi-step actions)
struct SequenceStep {
    std::string key;
    int duration_ms;
    int wait_ms;  // pause AFTER this step before next
};

std::vector<SequenceStep> GetSequence(const std::string& name) {
    std::string n = ToLower(name);
    if (n == "interact") {
        return {{"confirm", 100, 50}};
    }
    if (n == "open_menu") {
        return {{"menu", 100, 50}};
    }
    if (n == "close_menu") {
        return {{"cancel", 100, 50}};
    }
    if (n == "pause") {
        return {{"pause", 100, 50}};
    }
    if (n == "minimap") {
        return {{"minimap", 100, 50}};
    }
    if (n == "navigate_up") {
        return {{"up", 300, 50}};
    }
    if (n == "navigate_down") {
        return {{"down", 300, 50}};
    }
    if (n == "navigate_left") {
        return {{"left", 300, 50}};
    }
    if (n == "navigate_right") {
        return {{"right", 300, 50}};
    }
    if (n == "select") {
        return {{"confirm", 100, 100}};
    }
    if (n == "back") {
        return {{"cancel", 100, 100}};
    }
    return {};
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
    if (k == "confirm") return SDLK_RETURN;
    if (k == "cancel") return SDLK_ESCAPE;
    if (k == "menu")   return SDLK_ESCAPE;
    if (k == "pause")  return SDLK_p;
    if (k == "minimap") return SDLK_TAB;
    if (k == "escape" || k == "esc") return SDLK_ESCAPE;
    if (k == "return" || k == "enter") return SDLK_RETURN;
    if (k == "tab") return SDLK_TAB;
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

void InputInjector::_QueueActionUnlocked(const std::string& key, int duration_ms) {
    KeyAction action;
    action.key = ToLower(key);
    action.duration_ms = duration_ms;
    action.pressed = false;
    action.press_time = 0;
    _actions.push_back(action);
}

void InputInjector::QueueAction(const std::string& key, int duration_ms) {
    SDL_LockMutex(_mutex);
    _QueueActionUnlocked(key, duration_ms);
    _current_sequence.clear();  // interrupt any running sequence
    SDL_UnlockMutex(_mutex);
}

void InputInjector::QueueSequence(const std::string& name) {
    SDL_LockMutex(_mutex);
    auto steps = GetSequence(name);
    if (steps.empty()) {
        SDL_UnlockMutex(_mutex);
        return;
    }
    for (const auto& step : steps) {
        _QueueActionUnlocked(step.key, step.duration_ms);
        if (step.wait_ms > 0) {
            // Insert a no-op wait action to consume time
            KeyAction wait;
            wait.key = "__wait__";
            wait.duration_ms = step.wait_ms;
            wait.pressed = true;  // already "done" — will be skipped in Update
            wait.press_time = 0;
            _actions.push_back(wait);
        }
    }
    _current_sequence = ToLower(name);
    SDL_UnlockMutex(_mutex);
}

std::string InputInjector::GetCurrentSequence() const {
    SDL_LockMutex(_mutex);
    std::string seq = _current_sequence;
    SDL_UnlockMutex(_mutex);
    return seq;
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
        // Wait actions — just consume time then drop
        if (action.key == "__wait__") {
            if (!action.pressed) {
                action.pressed = true;
                action.press_time = now;
                remaining.push_back(action);
            } else if (static_cast<int>(now - action.press_time) >= action.duration_ms) {
                // wait complete — drop it, clear sequence if this was the last real action
            } else {
                remaining.push_back(action);
            }
            continue;
        }

        SDL_Keycode keycode = _KeyNameToSDL(action.key);
        if (keycode == SDLK_UNKNOWN) continue;

        if (!action.pressed) {
            _InjectKeyDown(keycode);
            action.pressed = true;
            action.press_time = now;
            remaining.push_back(action);
        } else {
            if (static_cast<int>(now - action.press_time) >= action.duration_ms) {
                _InjectKeyUp(keycode);
            } else {
                remaining.push_back(action);
            }
        }
    }

    _actions.swap(remaining);
    if (_actions.empty()) {
        _current_sequence.clear();
    }
    SDL_UnlockMutex(_mutex);
}

void InputInjector::Clear() {
    SDL_LockMutex(_mutex);
    _actions.clear();
    _current_sequence.clear();
    SDL_UnlockMutex(_mutex);
}

bool InputInjector::IsActive() const {
    SDL_LockMutex(_mutex);
    bool active = !_actions.empty();
    SDL_UnlockMutex(_mutex);
    return active;
}

} // namespace vt_mod
