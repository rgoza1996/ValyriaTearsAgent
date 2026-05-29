////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — HTTP Server Implementation (civetweb)
// Listens on port 8080, exposes REST endpoints for bot control
////////////////////////////////////////////////////////////////////////////////

#include "http_server.h"
#include "game_api.h"
#include "input_inject.h"
#include "dashboard_html.h"
#include <civetweb.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>

namespace vt_mod {

namespace {

bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

// Minimal JSON parser for action requests
std::string extract_json_field(const std::string& json_str, const std::string& field) {
    std::string pattern = "\"" + field + "\"";
    size_t pos = json_str.find(pattern);
    if (pos == std::string::npos) return "";

    size_t colon = json_str.find(':', pos);
    if (colon == std::string::npos) return "";

    size_t start = colon + 1;
    while (start < json_str.size() && (json_str[start] == ' ' || json_str[start] == '\t')) ++start;

    if (start >= json_str.size()) return "";

    if (json_str[start] == '"') {
        size_t end = json_str.find('"', start + 1);
        if (end == std::string::npos) return "";
        return json_str.substr(start + 1, end - start - 1);
    } else {
        size_t end = start;
        while (end < json_str.size() && (std::isdigit(json_str[end]) || json_str[end] == '.' || json_str[end] == '-')) ++end;
        return json_str.substr(start, end - start);
    }
}

std::string to_string(int v) {
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

// Simple base64 encode (for /screenshot_base64)
std::string base64_encode(const std::string& input) {
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (size_t i = 0; i < input.size(); i += 3) {
        int b0 = static_cast<unsigned char>(input[i]);
        int b1 = (i + 1 < input.size()) ? static_cast<unsigned char>(input[i + 1]) : 0;
        int b2 = (i + 2 < input.size()) ? static_cast<unsigned char>(input[i + 2]) : 0;
        out += b64[b0 >> 2];
        out += b64[((b0 & 3) << 4) | (b1 >> 4)];
        out += (i + 1 < input.size()) ? b64[((b1 & 15) << 2) | (b2 >> 6)] : '=';
        out += (i + 2 < input.size()) ? b64[b2 & 63] : '=';
    }
    return out;
}

void send_response(struct mg_connection* conn, int status, const std::string& content_type, const std::string& body) {
    std::string status_str = (status == 200) ? "200 OK" : (status == 400 ? "400 Bad Request" : "404 Not Found");
    mg_printf(conn, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",
        status_str.c_str(), content_type.c_str(), body.size());
    mg_write(conn, body.data(), body.size());
}

} // anonymous namespace

HTTPServer::HTTPServer() : _ctx(nullptr), _port(0) {}

HTTPServer* HTTPServer::SingletonGet() {
    static HTTPServer inst;
    return &inst;
}

HTTPServer::~HTTPServer() {
    Stop();
}

bool HTTPServer::Start(int port) {
    if (_ctx) return true; // already running

    _port = port;
    const char* options[] = {
        "listening_ports", (":" + to_string(port)).c_str(),
        "request_timeout_ms", "5000",
        "enable_keep_alive", "no",
        "num_threads", "1",
        nullptr
    };

    _ctx = mg_start(nullptr, this, options);
    if (!_ctx) return false;

    // Register our handler for all routes
    mg_set_request_handler(_ctx, "/", _HandleRequest, this);
    return true;
}

void HTTPServer::Stop() {
    if (_ctx) {
        mg_stop(_ctx);
        _ctx = nullptr;
    }
}

// static
int HTTPServer::_HandleRequest(struct mg_connection* conn, void* /*cbdata*/) {
    const mg_request_info* info = mg_get_request_info(conn);
    if (!info || !info->request_method) {
        send_response(conn, 400, "text/plain", "Bad request");
        return 1;
    }

    std::string uri = info->uri ? info->uri : "";
    std::string method = info->request_method;

    // GET /health
    if (method == "GET" && uri == "/health") {
        std::string body = "{\"status\":\"ok\",\"server\":\"valyria-tear\"}";
        send_response(conn, 200, "application/json", body);
        return 1;
    }

    // GET /state
    if (method == "GET" && uri == "/state") {
        std::string body = GameAPI::SingletonGet()->GetStateJSON();
        send_response(conn, 200, "application/json", body);
        return 1;
    }

    // GET /screenshot -> PNG file
    if (method == "GET" && (uri == "/screenshot" || uri == "/screenshot.png")) {
        std::string path = GameAPI::SingletonGet()->TakeScreenshot();
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            send_response(conn, 404, "text/plain", "Screenshot not found");
            return 1;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string data(size, ' ');
        if (!file.read(&data[0], size)) {
            send_response(conn, 500, "text/plain", "Failed to read screenshot");
            return 1;
        }
        send_response(conn, 200, "image/png", data);
        return 1;
    }

    // GET /screenshot_base64 -> JSON with base64 image
    if (method == "GET" && uri == "/screenshot_base64") {
        std::string path = GameAPI::SingletonGet()->TakeScreenshot();
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            send_response(conn, 404, "text/plain", "Screenshot not found");
            return 1;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string data(size, ' ');
        if (!file.read(&data[0], size)) {
            send_response(conn, 500, "text/plain", "Failed to read screenshot");
            return 1;
        }
        std::string body = "{\"status\":\"ok\",\"image\":\"" + base64_encode(data) + "\"}";
        send_response(conn, 200, "application/json", body);
        return 1;
    }

    // POST /action -> key action
    if (method == "POST" && uri == "/action") {
        std::string body;
        char buf[4096];
        int n = mg_read(conn, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            body = buf;
        }

        std::string key = extract_json_field(body, "key");
        std::string dur_str = extract_json_field(body, "duration_ms");
        int duration = 200;
        if (!dur_str.empty()) duration = std::atoi(dur_str.c_str());

        if (!key.empty()) {
            InputInjector::SingletonGet()->QueueAction(key, duration);
        }

        std::string response = "{\"status\":\"ok\",\"action\":{\"key\":\"" + key +
            "\",\"duration_ms\":" + to_string(duration) + "}}";
        send_response(conn, 200, "application/json", response);
        return 1;
    }

    // GET /dashboard -> HTML dashboard
    if (method == "GET" && (uri == "/dashboard" || uri == "/dashboard/")) {
        send_response(conn, 200, "text/html", DASHBOARD_HTML);
        return 1;
    }

    // 404
    send_response(conn, 404, "text/plain", "Not found: " + uri);
    return 1;
}

} // namespace vt_mod