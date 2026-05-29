# ValyriaTearsAgent

HTTP API bridge for ValyriaTear (1.1.0) — exposes game state and accepts input commands via a REST API, enabling AI agents to interact with the game.

## What it does

- **HTTP server** on port 8080 with REST endpoints
- **Game state API** — read current mode, party, position, quest progress
- **Input injection** — send keypresses and mouse clicks to the game
- **Screenshot capture** — grab the current game frame

## Architecture

```
ValyriaTear binary (mod linked)
├── src/mod/http_server.cpp   — POSIX socket HTTP server (no external deps)
├── src/mod/game_api.cpp      — Game state extraction via VT internals
├── src/mod/input_inject.cpp  — SDL2 key/mouse event injection
└── bin/etb                   — Compiled binary (ValyriaTear + mod)
```

## Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/health` | Server health check |
| GET | `/state` | Current game state (mode, party, position) |
| POST | `/action` | Inject input `{ "key": "down", "duration_ms": 300 }` |
| GET | `/screenshot` | PNG screenshot of current frame |
| GET | `/screenshot_base64` | Base64-encoded screenshot JSON |

## Build

```bash
cd ValyriaTear-1.1.0
make -j$(nproc)
```

Requires: C++17, SDL2, OpenGL, pthread

## Running

```bash
# Headless (Xvfb)
Xvfb :99 -screen 0 1024x768x24 &
export DISPLAY=:99
./bin/etb
```

Then hit the API:
```bash
curl http://localhost:8080/health
curl http://localhost:8080/state
curl -X POST http://localhost:8080/action -H "Content-Type: application/json" \
  -d '{"key":"down","duration_ms":300}'
```

## Key findings (dev journal)

- `civetweb` embedded server (v1.16) fails silently at `mg_start()` — replaced with POSIX socket server
- Game **must** be run from `ValyriaTear-1.1.0/` directory (CWD sensitivity)
- `SDL_VIDEODRIVER=dummy` causes OpenGL init failure — leave unset, use Xvfb instead
- Civetweb's `mg_start()` returns `nullptr` with no error logging in this environment

See [DEVELOPMENT.md](./DEVELOPMENT.md) for the full dev journal.

## License

MIT — same as ValyriaTear itself.