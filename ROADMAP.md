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

**Tested:** Slot 0 save → file created, /saves confirms exists.

---

## Priority 2 — Action Primitives

**Status: ✅ Done**

Replace raw SDL scancodes with named game actions.

**Before:**
```json
{ "key": "down", "duration_ms": 200 }
```

**After — `/do` endpoint:**
```json
{ "action": "interact" }
{ "action": "open_menu" }
{ "action": "close_menu" }
{ "action": "pause" }
{ "action": "navigate_up" }
{ "action": "navigate_down" }
{ "action": "navigate_left" }
{ "action": "navigate_right" }
{ "action": "select" }
{ "action": "back" }
```

**Raw key still supported** via `{"key": "...", "duration_ms": ...}` for direct control.

**Available keys:** up, down, left, right, confirm, cancel, menu, pause, minimap, escape, return, tab

**Endpoints:**
- `POST /action` — raw keypress (backward compatible)
- `POST /do` — named action primitives

**Tested:** All 11 primitives confirmed returning `{"status":"ok","action":"..."}`.

---

## Priority 3 — WebSocket Streaming

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

## Priority 4 — Hermes Agent Skill

**Status: Not Started**

Create a `valyriatear-player` skill so you can say "Play ValyriaTear" and Hermes sets everything up and starts playing autonomously.

Pattern from `pokemon-player` skill:
- Install/setup the game
- Start Xvfb
- Launch the server
- Begin autonomous gameplay
- Track objectives across sessions via persistent memory

---

## Priority 5 — Structured Memory Reading

**Status: Not Started**

Instead of inferring game state from screenshots, read actual C++ game variables.

**Current:** `/state` returns partial data (mode, party_size, map_name)

**Desired:** Full game state parsed from RAM/game structs:
- Player position (x, y, map_id)
- Inventory (items, quantities)
- Quest state (current objective, flags)
- NPC dialog text
- Battle state (enemy HP, available moves)

**Approach:** Find relevant structs in ValyriaTear source (GlobalManager, ModeManager, etc.) and expose them via GameAPI.

---

## Priority 6 — A* Pathfinding

**Status: Not Started**

**Use case:** Agent needs to navigate from point A to point B on a map.

**Requirements:**
1. Parse tilemap data from ValyriaTear's map format
2. Build a navigable grid (walkable vs blocked tiles)
3. Implement A* pathfinding
4. Expose `navigate_to(x, y)` action that follows the path

**Reference:** pokemon-agent has `pathfinding.py` with A* already implemented.

---

## Priority 7 — Multi-game / Multi-mod Framework

**Status: Not Started**

Generalize the architecture so ValyriaTear-agent becomes a pattern for other game bots.

**Abstractions needed:**
- `GameStateReader` — read game memory → JSON
- `InputInjector` — send inputs to game
- `EmulatorController` — save/load, screenshot, tick
- `GameServer` — REST + WebSocket API

This mirrors pokemon-agent's structure:
```
memory/reader.py     → abstract reader
memory/red.py       → concrete implementation
```

---

## Feature Comparison with pokemon-agent

| Feature | pokemon-agent | ValyriaTear-agent |
|---------|-------------|-------------------|
| Headless emulation | ✅ PyBoy/PyGBA | ✅ Xvfb + real binary |
| REST API | ✅ | ✅ |
| WebSocket | ✅ | ❌ Not yet |
| Save/Load state | ✅ | ✅ Priority 1 |
| Memory parsing | ✅ RAM → JSON | ⚠️ Partial |
| Action primitives | ✅ walk_to, interact | ✅ /do endpoint |
| A* pathfinding | ✅ | ❌ Not yet |
| Dashboard | ✅ + AI reasoning | ✅ (basic) |
| Hermes skill | ✅ | ❌ Not yet |
| Multi-game | ✅ 5 games | ❌ Single mod |
