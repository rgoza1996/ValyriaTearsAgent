#!/usr/bin/env python3
"""WebSocket proxy for ValyriaTear — streams screenshot frames + JSON state in memory."""

import asyncio
import json
import os
import struct
import subprocess
import sys
from io import BytesIO

try:
    import websockets
    from PIL import Image
except ImportError:
    print("ERROR: missing deps. Run: pip install websockets Pillow")
    sys.exit(1)

GAME_HOST = "localhost"
GAME_PORT = 8080
WS_PORT = 8765


def capture_screen_xwd():
    """Capture screenshot using xwd + PIL. Returns PNG bytes or None."""
    try:
        result = subprocess.run(
            ["xwd", "-root", "-silent"],
            env={**os.environ, "DISPLAY": ":99"},
            capture_output=True, timeout=5
        )
        if result.returncode != 0 or len(result.stdout) < 1024:
            return None

        data = result.stdout

        def r32(offset):
            return struct.unpack(">I", data[offset:offset+4])[0]

        header_words = r32(0)
        width = r32(16)
        height = r32(20)
        bpp = r32(44)
        stride = r32(48)

        if width < 2 or height < 2:
            return None

        pixel_offset = header_words * 4

        if bpp == 32:
            rows = []
            for y in range(height):
                row_start = pixel_offset + y * stride
                row_data = data[row_start:row_start + stride]
                rgb_row = bytearray(width * 3)
                for x in range(width):
                    rgb_row[x*3]   = row_data[x*4+2]
                    rgb_row[x*3+1] = row_data[x*4+1]
                    rgb_row[x*3+2] = row_data[x*4]
                rows.append(bytes(rgb_row))
            img = Image.frombytes("RGB", (width, height), b"".join(rows))
        elif bpp == 24:
            row_size = (width * 3 + 3) & ~3
            rows = []
            for y in range(height):
                row_start = pixel_offset + y * row_size
                row = data[row_start:row_start + row_size]
                rows.append(row[:width * 3])
            img = Image.frombytes("RGB", (width, height), b"".join(rows))
        else:
            return None

        buf = BytesIO()
        img.save(buf, format="PNG")
        return buf.getvalue()
    except Exception:
        return None


async def fetch_game_raw(path):
    """Fetch raw response body from game HTTP server."""
    reader, writer = await asyncio.open_connection(GAME_HOST, GAME_PORT)
    req = f"GET {path} HTTP/1.1\r\nHost: {GAME_HOST}\r\nConnection: close\r\n\r\n"
    writer.write(req.encode())
    await writer.drain()
    data = await asyncio.wait_for(reader.read(1024 * 1024), timeout=5)
    writer.close()
    await writer.wait_closed()
    if b"\r\n\r\n" in data:
        return data.split(b"\r\n\r\n", 1)[1]
    return b""


async def stream_to_client(ws):
    """Stream PNG frame + JSON state to client every 500ms. No disk I/O."""
    from websockets.exceptions import ConnectionClosed
    try:
        while True:
            # Capture and send PNG frame directly from memory
            png_bytes = await asyncio.get_event_loop().run_in_executor(None, capture_screen_xwd)
            if png_bytes:
                await ws.send(png_bytes)

            # Send JSON state
            state_bytes = await fetch_game_raw("/state")
            if state_bytes:
                try:
                    state = json.loads(state_bytes.decode("utf-8", errors="replace"))
                    await ws.send(json.dumps(state))
                except (json.JSONDecodeError, UnicodeError):
                    pass

            await asyncio.sleep(0.5)
    except ConnectionClosed:
        pass


async def handler(ws):
    await ws.send(json.dumps({"type": "connected", "msg": "ValyriaTear stream active"}))
    await stream_to_client(ws)


async def main():
    async with websockets.serve(handler, "0.0.0.0", WS_PORT):
        print(f"WebSocket proxy listening on ws://0.0.0.0:{WS_PORT}")
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
