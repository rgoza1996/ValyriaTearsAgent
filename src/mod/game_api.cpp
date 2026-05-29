////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Game State API Implementation
// Interfaces with ModeManager, GlobalManager, and VideoEngine
////////////////////////////////////////////////////////////////////////////////

#include "game_api.h"
#include "engine/video/video.h"
#include "engine/mode_manager.h"
#include "common/global/global.h"
#include <sstream>
#include <iomanip>

namespace vt_mod {

GameAPI::GameAPI() : _last_screenshot("/tmp/vt_screenshot.png") {}

GameAPI* GameAPI::SingletonGet() {
    static GameAPI inst;
    return &inst;
}

std::string GameAPI::TakeScreenshot() {
    vt_video::VideoManager->MakeScreenshot(_last_screenshot);
    return _last_screenshot;
}

std::string GameAPI::GetStateJSON() {
    using namespace vt_mode_manager;
    using namespace vt_global;
    using namespace vt_video;

    std::ostringstream json;

    // Get current mode
    uint8_t mode_type = ModeEngine::SingletonGet()->GetGameType();

    // Mode ID to name mapping
    const char* mode_names[] = {
        "dummy", "boot", "map", "battle", "menu", "shop", "pause", "save"
    };
    const char* mode_name = (mode_type >= 0 && mode_type <= 7) ? mode_names[mode_type] : "unknown";

    json << "{\"status\":\"ok\",";
    json << "\"mode\":\"" << mode_name << "\",";
    json << "\"mode_id\":" << static_cast<int>(mode_type) << ",";

    // Get party info from GlobalManager
    try {
        CharacterHandler& char_handler = GlobalManager->GetCharacterHandler();
        GlobalParty& party = char_handler.GetActiveParty();
        uint32_t party_size = party.GetPartySize();
        const std::vector<GlobalCharacter*>& chars = party.GetAllCharacters();

        json << "\"party_size\":" << party_size << ",";
        json << "\"party\":[";

        for (uint32_t i = 0; i < party_size; ++i) {
            if (i > 0) json << ",";
            GlobalCharacter* actor = chars[i];
            json << "{";
            json << "\"hp\":" << actor->GetHitPoints() << ",";
            json << "\"max_hp\":" << actor->GetMaxHitPoints() << ",";
            json << "\"mp\":" << actor->GetManaPoints() << ",";
            json << "\"max_mp\":" << actor->GetMaxManaPoints();
            json << "}";
        }
        json << "]";
    } catch (const std::exception& e) {
        json << "\"party_size\":0,\"party\":[]";
    }

    json << "}";
    return json.str();
}

void GameAPI::SaveGame(int slot) {
    // TODO: invoke Lua save function via ScriptManager
}

void GameAPI::LoadGame(int slot) {
    // TODO: invoke Lua load function via ScriptManager
}

} // namespace vt_mod