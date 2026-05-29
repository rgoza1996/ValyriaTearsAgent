# ValyriaTearsAgent

HTTP API bridge for ValyriaTear (1.1.0) — exposes game state and accepts input commands via a REST API, enabling AI agents to interact with the game autonomously.

## Quick Start

```bash
# 1. Start Xvfb (must match game's video settings: 800x600)
Xvfb :99 -screen 0 800x600x24 &

# 2. Run the game
cd ValyriaTear-1.1.0
DISPLAY=:99 ./src/valyriatear

# 3. Access the dashboard
open http://localhost:8080/dashboard/
```

## Architecture

```
valyriatear binary (main.cpp)
  ├── VideoEngine      → MakeScreenshot() → PNG via xwd
  ├── InputEngine      → EventHandler() → SDL events
  ├── InputInjector    → QueueAction() → SDL_PushEvent()
  ├── GameAPI          → GetStateJSON() → mode, party, map_name
  └── HTTPServer        → POSIX socket server on :8080
        ├── GET  /health             → OK
        ├── GET  /state              → JSON game state
        ├── GET  /screenshot         → PNG image
        ├── GET  /screenshot_base64  → {"status":"ok","image":"..."}
        ├── GET  /dashboard/         → Live game dashboard (HTML)
        └── POST /action             → inject keypress
```

## Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/health` | Server health check |
| GET | `/state` | Current game state (mode, party_size, map_name) |
| GET | `/screenshot_base64` | Base64 PNG of current frame as JSON |
| GET | `/saves` | List available save slots |
| GET | `/dashboard/` | Live game dashboard (HTML/JS) |
| POST | `/action` | Inject input |
| POST | `/save` | Save game to slot (0-9) |
| POST | `/load` | Load game from slot (0-9) |

### POST /action

```bash
curl -X POST http://localhost:8080/action \
  -H "Content-Type: application/json" \
  -d '{"key": "confirm", "duration_ms": 200}'
```

Available keys: `up`, `down`, `left`, `right`, `confirm`, `cancel`, `menu`, `pause`

## Build

```bash
cd ValyriaTear-1.1.0
cmake -B src/CMakeFiles -DCMAKE_BUILD_TYPE=Release
make -C src/CMakeFiles -j$(nproc)
```

Requires: C++17, SDL2, OpenGL, pthread, Python3 (for screenshot capture)

## Dashboard

The dashboard at `/dashboard/` provides:

- **Live game view** — 800×600 screenshot, refreshed every 2s
- **Game state panel** — current mode, map, party size
- **D-pad controls** — click to inject inputs
- **API log** — real-time request/response trace

CSS: dark cyberpunk theme with cyan neon accents.

## Configuration

| Setting | Value | Location |
|---------|-------|----------|
| Xvfb display | `:99` | Set `DISPLAY=:99` before launch |
| Xvfb resolution | `800×600×24` | Must match `data/config/settings.lua` |
| HTTP port | `8080` | Hardcoded in `HTTPServer::Start()` |
| Game directory | `ValyriaTear-1.1.0/` | CWD — game won't find `data/` otherwise |

## Key Findings

- **Civetweb v1.16** — `mg_start()` fails silently; replaced with custom POSIX socket server
- **SDL_VIDEODRIVER=dummy** — causes OpenGL init failure; use Xvfb instead
- **Xvfb resolution** — must match game's `settings.lua` (800×600) to avoid letterboxing
- **Screenshot capture** — uses `xwd` piped through Python/PIL; handles both 24bpp and 32bpp XWD formats
- **Screenshot cleanup** — `/tmp/screenshot.png` and `_b64.txt` auto-deleted after 1 hour
- **Game crashes** — alpha software; watch for `/health` failures and restart as needed

## File Inventory (src/mod/)

| File | Purpose |
|------|---------|
| `http_server.cpp` + `.h` | POSIX socket HTTP server (no external deps) |
| `game_api.cpp` + `.h` | Game state via VideoManager, ModeManager, GlobalManager |
| `input_inject.cpp` + `.h` | Queued SDL keypress injection via SDL_PushEvent() |
| `dashboard.html` | Embedded dashboard UI (served at /dashboard/) |
| `dashboard_html.h` | Auto-generated C string literal from dashboard.html |
| `civetweb.c` + `.h` | Original (unused, retained for reference) |

## Development Journal

See [DEVELOPMENT.md](./DEVELOPMENT.md) for the full history: architecture decisions, bugs found and resolved, and next steps.

## License

MIT — same as ValyriaTear itself.
