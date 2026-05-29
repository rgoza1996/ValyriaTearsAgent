////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Input Injection
// Allows the HTTP server to inject virtual key events into the InputEngine
////////////////////////////////////////////////////////////////////////////////

#ifndef VALYRIA_INPUT_INJECT_H
#define VALYRIA_INPUT_INJECT_H

#include <string>
#include <vector>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mutex.h>

namespace vt_mod {

struct KeyAction {
    std::string key;       // "up", "down", "left", "right", "confirm", "cancel", "menu"
    int duration_ms;       // how long to hold the key
    bool pressed;          // whether key-down has been sent
    Uint32 press_time;     // SDL_GetTicks() when key was pressed
};

class InputInjector {
public:
    static InputInjector* SingletonGet();

    // Queue an action: key press for duration_ms milliseconds
    void QueueAction(const std::string& key, int duration_ms);

    // Process pending actions — call once per frame in main loop
    void Update();

    // Clear all pending actions
    void Clear();

    // Check if any action is currently active
    bool IsActive() const;

    // Queue a named sequence of actions
    void QueueSequence(const std::string& name);

    // Check current sequence name (empty = no sequence running)
    std::string GetCurrentSequence() const;

private:
    InputInjector();
    ~InputInjector();

    SDL_Keycode _KeyNameToSDL(const std::string& key);
    void _InjectKeyDown(SDL_Keycode key);
    void _InjectKeyUp(SDL_Keycode key);
    void _QueueActionUnlocked(const std::string& key, int duration_ms);

    std::vector<KeyAction> _actions;
    std::string _current_sequence;
    SDL_mutex* _mutex;
};

} // namespace vt_mod

#endif // VALYRIA_INPUT_INJECT_H
