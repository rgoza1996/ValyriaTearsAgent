////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Input Injection Implementation
////////////////////////////////////////////////////////////////////////////////

#include "input_inject.h"
#include <SDL2/SDL_timer.h>
#include "engine/input.h"
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

} // anonymous namespace

namespace {

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

// Static singleton accessor
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
    { FILE* _dbg = fopen("/tmp/inject_debug.log", "a"); if(_dbg) { fprintf(_dbg, "INJECT_KEY_DOWN: sym=%d\n", key); fclose(_dbg); } }
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
    { FILE* _dbg = fopen("/tmp/inject_debug.log", "a"); if(_dbg) { fprintf(_dbg, "INJECT_KEY_DOWN: sym=%d\n", key); fclose(_dbg); } }
    ev.key.keysym.mod = KMOD_NONE;
    SDL_PushEvent(&ev);
}

void InputInjector::QueueAction(const std::string& key, int duration_ms) {
    { FILE* _dbg = fopen("/tmp/inject_debug.log", "a"); if(_dbg) { fprintf(_dbg, "QueueAction: key=%s duration=%d\n", key.c_str(), duration_ms); fclose(_dbg); } }
    SDL_LockMutex(_mutex);
    { FILE* _dbg = fopen("/tmp/inject_debug.log", "a"); if(_dbg) { fprintf(_dbg, "QueueAction: LOCKED _actions.size()=%zu\n", _actions.size()); fclose(_dbg); } }
    _QueueActionUnlocked(key, duration_ms);
    { FILE* _dbg = fopen("/tmp/inject_debug.log", "a"); if(_dbg) { fprintf(_dbg, "QueueAction: AFTER _actions.size()=%zu\n", _actions.size()); fclose(_dbg); } }
    _current_sequence.clear();
    SDL_UnlockMutex(_mutex);
}

void InputInjector::_QueueActionUnlocked(const std::string& key, int duration_ms) {
    KeyAction action;
    action.key = ToLower(key);
    action.duration_ms = duration_ms;
    action.pressed = false;
    action.press_time = 0;
    _actions.push_back(action);
}

void InputInjector::Update() {
    { FILE* _dbg = fopen("/tmp/inject_debug.log", "a"); if(_dbg) { fprintf(_dbg, "Update: %zu actions\n", _actions.size()); fclose(_dbg); } }
    if (_actions.empty()) return;

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
            remaining.push_back(action);  // keep tracking for release
        } else {
            // Key is pressed — check if duration elapsed
            if (static_cast<int>(now - action.press_time) >= action.duration_ms) {
                _InjectKeyUp(keycode);
                // don't keep — action complete
            } else {
                bool already_in_remaining = false; for (const auto& a : remaining) { if (a.key == action.key && a.pressed) { already_in_remaining = true; break; } } if (!already_in_remaining) remaining.push_back(action);
            }
        }
    }

    _actions.swap(remaining);
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
            KeyAction wait;
            wait.key = "__wait__";
            wait.duration_ms = step.wait_ms;
            wait.pressed = true;
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

void InputInjector::Clear() {
    _actions.clear();
}

} // namespace vt_mod