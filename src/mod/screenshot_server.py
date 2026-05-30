#!/usr/bin/env python3
"""Screenshot capture for ValyriaTear HTTP server.
Usage modes:
  mode=serve  : HTTP server on PORT, writes unique PNG to /tmp/vt_screenshots/, prints "OK:size\n"
  mode=capture: capture once, write PNG to /tmp/screenshot.png (legacy HTTP compatibility)
"""
import json
import os
import struct
import subprocess
import sys
import time
from io import BytesIO
from PIL import Image

SCREENSHOT_DIR = "/tmp/vt_screenshots"
MAX_CACHED = 10

MODE = os.environ.get("CAPTURE_MODE", "capture")


def ensure_dir():
    if not os.path.exists(SCREENSHOT_DIR):
        os.makedirs(SCREENSHOT_DIR)


def prune_cached():
    """Keep only the last MAX_CACHED files by mtime."""
    ensure_dir()
    files = sorted(
        [os.path.join(SCREENSHOT_DIR, f) for f in os.listdir(SCREENSHOT_DIR) if f.endswith(".png")],
        key=os.path.getmtime,
        reverse=True
    )
    for f in files[MAX_CACHED:]:
        try:
            os.unlink(f)
        except Exception:
            pass


def capture_xwd_to_png():
    """Capture screen via xwd, return PNG bytes."""
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


def mode_serve(port=8081):
    """HTTP server: captures PNG on GET /capture, returns "OK:size\n" with PNG data after headers."""
    from http.server import HTTPServer, BaseHTTPRequestHandler

    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path == "/capture":
                png = capture_xwd_to_png()
                if png:
                    self.send_response(200)
                    self.send_header("Content-Type", "image/png")
                    self.send_header("Content-Length", len(png))
                    self.end_headers()
                    self.wfile.write(png)
                    # Prune old files after serving
                    prune_cached()
                else:
                    self.send_response(500)
                    self.end_headers()
                    self.wfile.write(b"capture failed")
            elif self.path == "/b64":
                png = capture_xwd_to_png()
                if png:
                    import base64
                    b64 = base64.b64encode(png).decode("ascii")
                    body = json.dumps({"status": "ok", "image": b64}).encode()
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.send_header("Content-Length", len(body))
                    self.end_headers()
                    self.wfile.write(body)
                    prune_cached()
                else:
                    self.send_response(500)
                    self.end_headers()
                    self.wfile.write(b"capture failed")
            else:
                self.send_response(404)
                self.end_headers()

        def log_message(self, fmt, *args):
            pass  # silent

    ensure_dir()
    server = HTTPServer(("127.0.0.1", port), Handler)
    print(f"Screenshot server listening on 127.0.0.1:{port}", file=sys.stderr)
    server.serve_forever()


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "serve":
        mode_serve()
    else:
        # Legacy mode: capture once, write PNG to /tmp/screenshot.png, print "OK:size"
        png = capture_xwd_to_png()
        if png:
            with open("/tmp/screenshot.png", "wb") as f:
                f.write(png)
            print(f"OK:{len(png)}")
        else:
            print("ERROR")
