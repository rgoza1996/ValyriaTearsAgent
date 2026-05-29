////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Game State API
// Exposes game state to the HTTP server via Lua bindings
////////////////////////////////////////////////////////////////////////////////

#ifndef VALYRIA_GAME_API_H
#define VALYRIA_GAME_API_H

#include <string>
#include <vector>

namespace vt_mod {

struct GameState {
    std::string mode;          // "boot", "menu", "map", "battle", "pause", "save", "shop"
    std::string map_name;
    float player_x;
    float player_y;
    int party_size;
    // Party: character 0 = hero
    std::vector<int> hp;
    std::vector<int> max_hp;
    std::vector<int> mp;
    std::vector<int> max_mp;
};

class GameAPI {
public:
    static GameAPI* SingletonGet();

    // Get full game state as JSON string
    std::string GetStateJSON();

    // Take screenshot, return path to PNG file
    std::string TakeScreenshot();

    // Trigger save
    void SaveGame(int slot = 1);

    // Trigger load
    void LoadGame(int slot = 1);

private:
    GameAPI();
    std::string _last_screenshot;
};

} // namespace vt_mod

#endif // VALYRIA_GAME_API_H