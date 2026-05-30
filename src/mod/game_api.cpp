////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — Game State API Implementation
////////////////////////////////////////////////////////////////////////////////

#include "game_api.h"
#include "engine/video/video.h"
#include "engine/mode_manager.h"
#include "common/global/global.h"
#include "common/global/actors/global_character.h"
#include "common/global/actors/global_party.h"
#include "common/global/objects/global_inventory_handler.h"
#include "common/global/quests/quests.h"
#include "common/global/quests/quest_log_entry.h"
#include "common/app_settings.h"
#include "modes/map/map_mode.h"
#include "modes/map/map_sprites/map_virtual_sprite.h"
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

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

void JsonEscape(const std::string& in, std::ostringstream& out) {
    for (char c : in) {
        if (c == '"') out << "\\\"";
        else if (c == '\\') out << "\\\\";
        else if (c == '\n') out << "\\n";
        else if (c == '\r') out << "\\r";
        else if (c == '\t') out << "\\t";
        else out << c;
    }
}

void WriteStr(std::ostringstream& json, const std::string& key, const std::string& value, bool last = false) {
    json << "\"" << key << "\":\"";
    JsonEscape(value, json);
    json << "\"";
    if (!last) json << ",";
}

void WriteRaw(std::ostringstream& json, const std::string& key, const std::string& value, bool last = false) {
    json << "\"" << key << "\":" << value;
    if (!last) json << ",";
}

void WriteInt(std::ostringstream& json, const std::string& key, int64_t value, bool last = false) {
    json << "\"" << key << "\":" << value;
    if (!last) json << ",";
}

void WriteFloat(std::ostringstream& json, const std::string& key, float value, bool last = false) {
    json << "\"" << key << "\":" << value;
    if (!last) json << ",";
}


// UTF-16 LE to UTF-8
std::string Utf16ToUtf8(const uint16_t* utf16, size_t len) {
    std::string result;
    result.reserve(len * 3 / 2);
    for (size_t i = 0; i < len; ++i) {
        uint16_t c = utf16[i];
        if (c == 0) break;
        if (c < 0x80) {
            result.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            result.push_back(static_cast<char>(0xC0 | (c >> 6)));
            result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xE0 | (c >> 12)));
            result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return result;
}

} // anonymous namespace


// ---------------------------------------------------------------------------
// State — enriched structured state
// ---------------------------------------------------------------------------

std::string GameAPI::GetStateJSON() {
    using namespace vt_mode_manager;
    using namespace vt_global;

    std::ostringstream json;
    json << "{";

    // Mode
    uint8_t mode_type = ModeManager->GetGameType();
    const char* mode_names[] = {
        "dummy", "boot", "map", "battle", "menu", "shop", "pause", "save"
    };
    const char* mode_name = (mode_type <= 7) ? mode_names[mode_type] : "unknown";
    WriteStr(json, "mode", std::string(mode_name));
    WriteInt(json, "mode_id", static_cast<int>(mode_type));

    // Map name + player position (only in map mode)
    if (mode_type == 2) { // MAP mode
        try {
            vt_map::MapMode* mm = vt_map::MapMode::CurrentInstance();
            if (mm) {
                vt_global::MapDataHandler& map_data = GlobalManager->GetMapData();
                std::string map_file = map_data.GetMapDataFilename();
                size_t slash = map_file.find_last_of("/\\");
                std::string map_name = (slash != std::string::npos) ? map_file.substr(slash + 1) : map_file;
                size_t dot = map_name.rfind(".lua");
                if (dot != std::string::npos) map_name = map_name.substr(0, dot);
                WriteStr(json, "map_name", map_name);

                vt_map::private_map::VirtualSprite* cam = mm->GetCamera();
                if (cam) {
                    float px = cam->GetXPosition();
                    float py = cam->GetYPosition();
                    FILE* dbg = fopen("/tmp/cam_debug.txt", "a");
                    if (dbg) { fprintf(dbg, "camera pos: %f, %f\n", px, py); fclose(dbg); }
                    WriteFloat(json, "player_x", px);
                    WriteFloat(json, "player_y", py);
                }
            }
        } catch (const std::exception& e) {
            // ignore
        }
    }

    // Party
    try {
        CharacterHandler& char_handler = GlobalManager->GetCharacterHandler();
        GlobalParty& party = char_handler.GetActiveParty();
        uint32_t party_size = party.GetPartySize();
        const std::vector<GlobalCharacter*>& chars = party.GetAllCharacters();

        WriteInt(json, "party_size", static_cast<int>(party_size));
        json << "\"party\":[";
        for (uint32_t i = 0; i < party_size; ++i) {
            if (i > 0) json << ",";
            GlobalCharacter* actor = chars[i];
            json << "{";
            WriteInt(json, "id", actor->GetID());
            WriteInt(json, "hp", actor->GetHitPoints());
            WriteInt(json, "max_hp", actor->GetMaxHitPoints());
            WriteInt(json, "xp_level", actor->GetExperienceLevel(), true);
            json << "}";
        }
        json << "]";

        // Quests
        GameQuests& quests = GlobalManager->GetGameQuests();
        std::vector<QuestLogEntry*> active_quests = quests.GetActiveQuestIds();
        json << ",\"quests\":[";
        bool first_quest = true;
        for (QuestLogEntry* entry : active_quests) {
            if (!entry) continue;
            if (!first_quest) json << ",";
            first_quest = false;
            json << "{";
            WriteRaw(json, "quest_id", "\"" + entry->GetQuestId() + "\"");
            WriteInt(json, "log_number", entry->GetQuestLogNumber());
            bool is_last_quest = first_quest;
            try {
                const QuestLogInfo& info = quests.GetQuestInfo(entry->GetQuestId());
                const vt_utils::ustring& title = info.GetTitle();
                const vt_utils::ustring& desc = info.GetDescription();
                WriteStr(json, "title", Utf16ToUtf8(title.c_str(), title.size()));
                WriteStr(json, "description", Utf16ToUtf8(desc.c_str(), desc.size()), is_last_quest);
            } catch (const std::exception&) {
                WriteStr(json, "title", "", is_last_quest);
            }
            json << "}";
        }
        json << "]";

        // Inventory
        InventoryHandler& inv = GlobalManager->GetInventoryHandler();
        auto& items = inv.GetInventoryItems();
        json << ",\"inventory_count\":" << static_cast<int>(items.size());

        // Gold
        json << ",\"gold\":" << GlobalManager->GetDrunes();

    } catch (const std::exception& e) {
        WriteInt(json, "party_size", 0, true);
        json << "\"party\":[]";
    }

    json << "}";
    return json.str();
}

// ---------------------------------------------------------------------------
// Saves
// ---------------------------------------------------------------------------

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

    ModeManager->PopAll();

    bool result = GlobalManager->LoadGame(filename.str(), slot);

    if (!result) {
        return "{\"status\":\"error\",\"message\":\"Load failed\"}";
    }

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
