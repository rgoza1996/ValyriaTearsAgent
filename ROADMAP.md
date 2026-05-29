# ValyriaTear Agent — Roadmap

Inspired by [pokemon-agent](https://github.com/NousResearch/pokemon-agent) by Nous Research.

## Priority 1 — Save/Load State

**Status: In Progress**

The ValyriaTear game crashes frequently (alpha software). Named save states would let the agent:
- Create checkpoints before risky encounters
- Restore and retry after crashes
- Test strategies without losing progress

**API design:**
```
POST /save     { "name": "before_boss" }  → saves to ~/.valyriatear/saves/<name>.save
POST /load     { "name": "before_boss" }  → restores from save
GET  /saves    → list of available saves
```

**Implementation:** ValyriaTear already has a save format. Need to find the save/load functions in the C++ codebase and expose them via the HTTP API.

---

## Priority 2 — Action Primitives

**Status: Not Started**

Replace raw SDL scancodes with named game actions.

**Current:** `POST /action { "key": "down", "duration_ms": 200 }`

**Proposed:**
```json
{ "action": "walk_to", "x": 5, "y": 3 }
{ "action": "interact" }
{ "action": "open_menu" }
{ "action": "close_menu" }
{ "action": "navigate_to", "target": "Save Point" }
```

**Benefits:** Agent can issue intent-based commands rather than pixel-counting navigation.

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
| Save/Load state | ✅ | ❌ Not yet |
| Memory parsing | ✅ RAM → JSON | ⚠️ Partial |
| Action primitives | ✅ walk_to, interact | ❌ Raw keys only |
| A* pathfinding | ✅ | ❌ Not yet |
| Dashboard | ✅ + AI reasoning | ✅ (basic) |
| Hermes skill | ✅ | ❌ Not yet |
| Multi-game | ✅ 5 games | ❌ Single mod |
