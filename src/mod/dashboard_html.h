// Auto-generated — do not edit manually
// Regen: python3 scripts/gen_dashboard_html.py > src/mod/dashboard_html.h

#ifndef VALYRIA_DASHBOARD_HTML_H
#define VALYRIA_DASHBOARD_HTML_H

static const char DASHBOARD_HTML[] = R"html(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ValyriaTear Agent</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            background: #0a0a0f;
            color: #e0e0ff;
            font-family: 'Courier New', monospace;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
        }
        header {
            background: linear-gradient(135deg, #1a1a2e, #16213e);
            border-bottom: 1px solid #0f3460;
            padding: 12px 20px;
            display: flex;
            align-items: center;
            gap: 16px;
        }
        header h1 {
            font-size: 16px;
            color: #00d4ff;
            text-shadow: 0 0 10px #00d4ff66;
            letter-spacing: 2px;
        }
        .status { font-size: 12px; color: #888; }
        .status.connected { color: #00ff88; }
        .status.disconnected { color: #ff4444; }
        main {
            display: grid;
            grid-template-columns: 1fr 320px;
            gap: 16px;
            padding: 16px;
            flex: 1;
        }
        .game-view {
            background: #111122;
            border: 1px solid #0f3460;
            border-radius: 8px;
            padding: 12px;
            display: flex;
            flex-direction: column;
            gap: 12px;
        }
        .game-screen-wrapper {
            position: relative;
            background: #000;
            border-radius: 4px;
            overflow: hidden;
        }
        .game-screen-frame {
            position: relative;
            display: inline-block;
            min-width: 320px;
        }
        .game-screen-frame::before,
        .game-screen-frame::after,
        .corner-br::before,
        .corner-br::after {
            content: '';
            position: absolute;
            width: 16px;
            height: 16px;
            border-color: #00d4ff;
            border-style: solid;
        }
        .game-screen-frame::before { top: 0; left: 0; border-width: 2px 0 0 2px; }
        .game-screen-frame::after  { top: 0; right: 0; border-width: 2px 2px 0 0; }
        .corner-br { position: absolute; bottom: 0; left: 0; right: 0; height: 16px; }
        .corner-br::before { bottom: 0; left: 0; border-width: 0 0 2px 2px; }
        .corner-br::after  { bottom: 0; right: 0; border-width: 0 2px 2px 0; }
        #gameScreen {
            display: block;
            max-width: 100%;
            min-height: 300px;
            background: #000;
        }
        .screen-overlay {
            position: absolute;
            inset: 0;
            background: rgba(0,0,0,0.7);
            display: flex;
            align-items: center;
            justify-content: center;
            color: #555;
            font-size: 14px;
        }
        .screen-overlay.hidden { display: none; }
        .state-panel {
            background: #0d0d1a;
            border-radius: 4px;
            padding: 10px;
        }
        .state-row {
            display: flex;
            justify-content: space-between;
            font-size: 12px;
            padding: 4px 0;
            border-bottom: 1px solid #1a1a2e;
        }
        .state-row:last-child { border: none; }
        .state-label { color: #666; }
        .state-value { color: #00d4ff; font-weight: bold; }
        .sidebar {
            display: flex;
            flex-direction: column;
            gap: 12px;
        }
        .controls {
            background: #111122;
            border: 1px solid #0f3460;
            border-radius: 8px;
            padding: 12px;
        }
        .controls h3 {
            font-size: 11px;
            color: #666;
            letter-spacing: 1px;
            margin-bottom: 10px;
        }
        .dpad {
            display: grid;
            grid-template-columns: repeat(3, 44px);
            grid-template-rows: repeat(3, 44px);
            gap: 4px;
            justify-content: center;
            margin-bottom: 16px;
        }
        .dpad-btn {
            background: #1a1a2e;
            border: 1px solid #0f3460;
            border-radius: 6px;
            color: #00d4ff;
            font-size: 16px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: background 0.1s, box-shadow 0.1s;
            user-select: none;
        }
        .dpad-btn:hover { background: #16213e; box-shadow: 0 0 8px #00d4ff44; }
        .dpad-btn:active { background: #0f3460; }
        .dpad-btn.placeholder { visibility: hidden; }
        .action-buttons {
            display: flex;
            flex-wrap: wrap;
            gap: 8px;
            justify-content: center;
        }
        .action-btn {
            background: #1a1a2e;
            border: 1px solid #0f3460;
            border-radius: 6px;
            color: #e0e0ff;
            font-size: 12px;
            padding: 8px 14px;
            cursor: pointer;
            transition: all 0.1s;
            font-family: inherit;
        }
        .action-btn:hover { background: #16213e; box-shadow: 0 0 8px #00d4ff44; }
        .action-btn:active { background: #0f3460; }
        .action-btn.confirm { border-color: #00ff88; color: #00ff88; }
        .action-btn.cancel  { border-color: #ff6b6b; color: #ff6b6b; }
        .api-debug {
            background: #111122;
            border: 1px solid #0f3460;
            border-radius: 8px;
            padding: 12px;
            flex: 1;
            overflow: hidden;
            display: flex;
            flex-direction: column;
        }
        .api-debug h3 {
            font-size: 11px;
            color: #666;
            letter-spacing: 1px;
            margin-bottom: 8px;
        }
        #apiLog {
            flex: 1;
            overflow-y: auto;
            font-size: 11px;
            color: #555;
            line-height: 1.6;
            word-break: break-all;
        }
        .api-log-entry { padding: 2px 0; border-bottom: 1px solid #1a1a2e; }
        .api-log-entry.ok    { color: #00ff88; }
        .api-log-entry.err   { color: #ff6b6b; }
        .api-log-entry.info  { color: #888; }
    </style>
</head>
<body>
<header>
    <h1>⚔ VALYRIA TEAR</h1>
    <span class="status disconnected" id="status">Connecting...</span>
</header>

<main>
    <div class="game-view">
        <div class="game-screen-wrapper">
            <div class="game-screen-frame">
                <img id="gameScreen" src="data:image/png;base64," alt="Game Screen">
                <div class="screen-overlay" id="screenOverlay">Waiting for game...</div>
                <div class="corner-br"></div>
            </div>
        </div>
        <div class="state-panel" id="statePanel">
            <div class="state-row"><span class="state-label">Mode</span><span class="state-value" id="stMode">—</span></div>
            <div class="state-row"><span class="state-label">Map</span><span class="state-value" id="stMap">—</span></div>
            <div class="state-row"><span class="state-label">Party</span><span class="state-value" id="stParty">—</span></div>
        </div>
    </div>

    <div class="sidebar">
        <div class="controls">
            <h3>CONTROLS</h3>
            <div class="dpad">
                <div class="dpad-btn placeholder"></div>
                <button class="dpad-btn" data-key="up">▲</button>
                <div class="dpad-btn placeholder"></div>
                <button class="dpad-btn" data-key="left">◀</button>
                <div class="dpad-btn placeholder"></div>
                <button class="dpad-btn" data-key="right">▶</button>
                <div class="dpad-btn placeholder"></div>
                <button class="dpad-btn" data-key="down">▼</button>
                <div class="dpad-btn placeholder"></div>
            </div>
            <div class="action-buttons">
                <button class="action-btn confirm" data-key="confirm" data-ms="200">Confirm</button>
                <button class="action-btn cancel" data-key="cancel" data-ms="200">Cancel</button>
                <button class="action-btn" data-key="menu" data-ms="200">Menu</button>
                <button class="action-btn" data-key="pause" data-ms="200">Pause</button>
            </div>
        </div>

        <div class="api-debug">
            <h3>API LOG</h3>
            <div id="apiLog"></div>
        </div>
    </div>
</main>

<script>
(function() {
    'use strict';
    var BASE = window.location.protocol + '//' + window.location.host;
    var POLL_MS = 1500;
    var hasFrame = false;
    var logEl = document.getElementById('apiLog');
    var statusEl = document.getElementById('status');

    function logMsg(text, type) {
        var el = document.createElement('div');
        el.className = 'api-log-entry ' + (type || 'info');
        var t = new Date().toLocaleTimeString();
        el.textContent = t + ' ' + text;
        logEl.appendChild(el);
        logEl.scrollTop = logEl.scrollHeight;
        while (logEl.children.length > 100) logEl.removeChild(logEl.firstChild);
    }

    function setStatus(ok, text) {
        statusEl.className = 'status ' + (ok ? 'connected' : 'disconnected');
        statusEl.textContent = text;
    }

    function sendAction(key, ms) {
        fetch(BASE + '/action', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ key: key, duration_ms: ms || 200 })
        })
        .then(function(r) { return r.json(); })
        .then(function(d) { logMsg('POST /action ' + key + ' → ok', 'ok'); })
        .catch(function(e) { logMsg('POST /action ' + key + ' ERROR: ' + e, 'err'); });
    }

    document.querySelectorAll('[data-key]').forEach(function(btn) {
        btn.addEventListener('click', function() {
            sendAction(btn.dataset.key, parseInt(btn.dataset.ms) || 200);
        });
    });

    async function poll() {
        try {
            var stateRes = await fetch(BASE + '/state');
            var imgRes = await fetch(BASE + '/screenshot_base64');
            if (!stateRes.ok || !imgRes.ok) throw new Error('fetch failed');

            var state = await stateRes.json();
            var img = await imgRes.json();

            if (!hasFrame && img.image) {
                hasFrame = true;
                document.getElementById('screenOverlay').classList.add('hidden');
            }
            if (img.image) {
                document.getElementById('gameScreen').src = 'data:image/png;base64,' + img.image;
            }
            document.getElementById('stMode').textContent = state.mode || '—';
            document.getElementById('stMap').textContent = state.map_name || '—';
            document.getElementById('stParty').textContent =
                state.party_size != null ? state.party_size + ' chars' : '—';

            setStatus(true, '● Connected');
        } catch (e) {
            setStatus(false, 'Server unreachable');
        }
    }

    poll();
    setInterval(poll, POLL_MS);
})();
</script>
</body>
</html>
)html";

#endif // VALYRIA_DASHBOARD_HTML_H