////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Game State API Implementation
////////////////////////////////////////////////////////////////////////////////

#include "game_api.h"
#include "engine/video/video.h"
#include "engine/mode_manager.h"
#include "common/global/global.h"
#include "common/global/actors/global_character.h"
#include "common/app_settings.h"
#include "modes/map/map_mode.h"
#include "common/global/maps/map_data_handler.h"
#include <sstream>
#include <sys/stat.h>

namespace vt_mod {

GameAPI::GameAPI() : _last_screenshot("/tmp/vt_screenshot.png") {
    _saves_dir = vt_common::GetUserDataPath();
}

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

    std::ostringstream json;

    uint8_t mode_type = ModeManager->GetGameType();

    const char* mode_names[] = {
        "dummy", "boot", "map", "battle", "menu", "shop", "pause", "save"
    };
    const char* mode_name = (mode_type <= 7) ? mode_names[mode_type] : "unknown";

    json << "{\"status\":\"ok\",";
    json << "\"mode\":\"" << mode_name << "\",";
    json << "\"mode_id\":" << static_cast<int>(mode_type);

    try {
        CharacterHandler& char_handler = GlobalManager->GetCharacterHandler();
        GlobalParty& party = char_handler.GetActiveParty();
        uint32_t party_size = party.GetPartySize();
        const std::vector<GlobalCharacter*>& chars = party.GetAllCharacters();

        json << ",\"party_size\":" << party_size;
        json << ",\"party\":[";

        for (uint32_t i = 0; i < party_size; ++i) {
            if (i > 0) json << ",";
            GlobalCharacter* actor = chars[i];
            json << "{";
            json << "\"id\":" << actor->GetID() << ",";
            json << "\"hp\":" << actor->GetHitPoints() << ",";
            json << "\"max_hp\":" << actor->GetMaxHitPoints();
            json << "}";
        }
        json << "]";
    } catch (const std::exception& e) {
        json << ",\"party_size\":0,\"party\":[]";
    }

    json << "}";
    return json.str();
}

std::string GameAPI::GetSavesDirectory() {
    return _saves_dir;
}

std::string GameAPI::ListSaves() {
    std::ostringstream json;
    json << "{\"status\":\"ok\",\"saves\":[";

    bool first = true;
    for (int i = 0; i < 10; ++i) {
        std::ostringstream path;
        path << _saves_dir << "saved_game_" << i << ".lua";

        struct stat st;
        bool exists = (stat(path.str().c_str(), &st) == 0);

        if (!first) json << ",";
        first = false;

        json << "{\"slot\":" << i << ","
             << "\"path\":\"" << path.str() << "\","
             << "\"exists\":" << (exists ? "true" : "false") << "}";
    }

    json << "]}";
    return json.str();
}

std::string GameAPI::SaveGame(int slot) {
    using namespace vt_global;

    if (slot < 0 || slot > 9) {
        std::ostringstream err;
        err << "{\"status\":\"error\",\"message\":\"Invalid slot " << slot << ", must be 0-9\"}";
        return err.str();
    }

    std::ostringstream filename;
    filename << _saves_dir << "saved_game_" << slot << ".lua";

    bool result = GlobalManager->SaveGame(filename.str(), slot, 0, 0);

    if (result) {
        std::ostringstream resp;
        resp << "{\"status\":\"ok\",\"slot\":" << slot << ",\"path\":\"" << filename.str() << "\"}";
        return resp.str();
    } else {
        return "{\"status\":\"error\",\"message\":\"Save failed\"}";
    }
}

std::string GameAPI::LoadGame(int slot) {
    using namespace vt_mode_manager;
    using namespace vt_global;

    if (slot < 0 || slot > 9) {
        std::ostringstream err;
        err << "{\"status\":\"error\",\"message\":\"Invalid slot " << slot << ", must be 0-9\"}";
        return err.str();
    }

    std::ostringstream filename;
    filename << _saves_dir << "saved_game_" << slot << ".lua";

    struct stat st;
    if (stat(filename.str().c_str(), &st) != 0) {
        std::ostringstream err;
        err << "{\"status\":\"error\",\"message\":\"No save found in slot " << slot << "\"}";
        return err.str();
    }

    // Pop all modes first
    ModeManager->PopAll();

    // Load game into GlobalManager
    bool result = GlobalManager->LoadGame(filename.str(), slot);

    if (!result) {
        return "{\"status\":\"error\",\"message\":\"Load failed\"}";
    }

    // Reconstruct and push MapMode
    try {
        vt_global::MapDataHandler& map_data = GlobalManager->GetMapData();
        vt_map::MapMode* mm = new vt_map::MapMode(
            map_data.GetMapDataFilename(),
            map_data.GetMapScriptFilename(),
            map_data.GetSaveStamina(),
            false
        );
        ModeManager->Push(mm, true, true);

        std::ostringstream resp;
        resp << "{\"status\":\"ok\",\"slot\":" << slot << "}";
        return resp.str();
    } catch (const std::exception& e) {
        return "{\"status\":\"error\",\"message\":\"Load succeeded but map restore failed\"}";
    }
}

} // namespace vt_mod
