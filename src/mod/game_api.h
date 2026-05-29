////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Game State API
// Exposes game state to the HTTP server
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

    // Save game to a named slot. slot: 0-based slot index.
    // Returns JSON: {"status":"ok","slot":1,"path":"/path/to/save.lua"}
    // or {"status":"error","message":"..."}
    std::string SaveGame(int slot);

    // Load game from a named slot.
    // Returns JSON: {"status":"ok","slot":1}
    // or {"status":"error","message":"..."}
    std::string LoadGame(int slot);

    // List all available save slots with metadata.
    // Returns JSON: {"saves":[{"slot":0,"path":"...","exists":true},...]}
    std::string ListSaves();

    // Get the save directory path
    std::string GetSavesDirectory();

private:
    GameAPI();
    std::string _last_screenshot;
    std::string _saves_dir;
};

} // namespace vt_mod

#endif // VALYRIA_GAME_API_H
