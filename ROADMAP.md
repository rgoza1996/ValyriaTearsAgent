# ValyriaTear Agent — Roadmap

Inspired by [pokemon-agent](https://github.com/NousResearch/pokemon-agent) by Nous Research.

## Priority 1 — Save/Load State

**Status: ✅ Done**

The ValyriaTear game crashes frequently (alpha software). Named save states let the agent:
- Create checkpoints before risky encounters
- Restore and retry after crashes
- Test strategies without losing progress

**API:**
```
GET  /saves              → list available slots
POST /save  {"slot": 0} → save to slot 0-9
POST /load  {"slot": 0} → restore from slot 0-9
```

**Saves location:** `~/.local/share/valyriatear/saved_game_<N>.lua`

---

## Priority 2 — Action Primitives

**Status: ✅ Done**

Replace raw SDL scancodes with named game actions.

**Endpoints:**
- `POST /action` — raw keypress: `{"key": "up", "duration_ms": 200}`
- `POST /do` — named primitives: `{"action": "interact"}`

**Available:** interact, open_menu, close_menu, pause, minimap, navigate_up/down/left/right, select, back

---

## Priority 3 — Structured Memory Reading

**Status: ✅ Done**

`GET /state` now returns enriched structured game state:

```json
{
  "mode": "map",
  "mode_id": 2,
  "map_name": "hero_home",
  "player_x": 4.5,
  "player_y": 12.3,
  "party_size": 2,
  "party": [
    {"id": 1, "hp": 45, "max_hp": 100, "xp_level": 3},
    {"id": 2, "hp": 30, "max_hp": 80,  "xp_level": 2}
  ],
  "quests": [
    {"quest_id": "darkness_falls", "log_number": 1}
  ],
  "inventory_count": 12,
  "gold": 350
}
```

**Fields added:** map_name, player_x/y (map mode), xp_level per character, quests array, inventory_count, gold (drunes).

**Limitation:** Quest title/description fields use `ustring` (UTF-16 internal) — not yet convertible to JSON strings without additional ustring.h dependency. Quest IDs are always available.

---

## Priority 4 — WebSocket Streaming

**Status: Not Started**

Replace polling with push-based WebSocket for lower latency and real-time events.

**Endpoints:**
- `ws://localhost:8080/ws` — push state + screenshot frames
- Live AI reasoning stream (pokemon-agent calls this "watch the agent think")

**Use cases:**
- Lower-latency game control
- Real-time action log
- Live dashboard updates without polling overhead

---

## Priority 5 — A* Pathfinding

**Status: Not Started**

**Use case:** Agent needs to navigate from point A to point B on a map.

**Requirements:**
1. Parse tilemap data from ValyriaTear's map format
2. Build a navigable grid (walkable vs blocked tiles)
3. Implement A* pathfinding
4. Expose `navigate_to(x, y)` action that follows the path

**Reference:** pokemon-agent has `pathfinding.py` with A* already implemented.

---

## Priority 6 — Hermes Agent Skill

**Status: Not Started**

Create a `valyriatear-player` skill so you can say "Play ValyriaTear" and Hermes sets everything up and starts playing autonomously.

Pattern from `pokemon-player` skill:
- Install/setup the game
- Start Xvfb
- Launch the server
- Begin autonomous gameplay
- Track objectives across sessions via persistent memory

---

## Priority 7 — Multi-game / Multi-mod Framework

**Status: Not Started**

Generalize the architecture so ValyriaTear-agent becomes a pattern for other game bots.

**Abstractions needed:**
- `GameStateReader` — read game memory → JSON
- `InputInjector` — send inputs to game
- `EmulatorController` — save/load, screenshot, tick
- `GameServer` — REST + WebSocket API

---

## Feature Comparison with pokemon-agent

| Feature | pokemon-agent | ValyriaTear-agent |
|---------|-------------|-------------------|
| Headless emulation | ✅ PyBoy/PyGBA | ✅ Xvfb + real binary |
| REST API | ✅ | ✅ |
| WebSocket | ✅ | ❌ Not yet |
| Save/Load state | ✅ | ✅ Priority 1 |
| Memory parsing | ✅ RAM → JSON | ✅ Priority 3 (partial) |
| Action primitives | ✅ walk_to, interact | ✅ Priority 2 |
| A* pathfinding | ✅ | ❌ Not yet |
| Dashboard | ✅ + AI reasoning | ✅ (basic) |
| Hermes skill | ✅ | ❌ Not yet |
| Multi-game | ✅ 5 games | ❌ Single mod |
