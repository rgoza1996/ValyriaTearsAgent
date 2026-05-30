// ValyriaTear HTTP API Mod — POSIX Socket HTTP Server
// Replaces civetweb mg_start which silently fails on this system

#include "http_server.h"
#include "game_api.h"
#include "input_inject.h"
#include "dashboard_html.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <thread>
#include <atomic>

namespace vt_mod {

namespace {

bool starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

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

int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void send_response(int client_fd, int status, const std::string& content_type, const std::string& body) {
    std::string status_str = (status == 200) ? "200 OK" : (status == 400 ? "400 Bad Request" : "404 Not Found");
    std::ostringstream header;
    header << "HTTP/1.1 " << status_str << "\r\n"
           << "Content-Type: " << content_type << "\r\n"
           << "Content-Length: " << body.size() << "\r\n"
           << "Connection: close\r\n"
           << "\r\n";
    std::string hdr = header.str();
    send(client_fd, hdr.data(), hdr.size(), 0);
    send(client_fd, body.data(), body.size(), 0);
}

void handle_request(int client_fd, const std::string& method, const std::string& uri, const std::string& body) {
    if (method == "GET" && uri == "/health") {
        send_response(client_fd, 200, "application/json", "{\"status\":\"ok\",\"server\":\"valyria-tear\"}");
        return;
    }
    if (method == "GET" && uri == "/state") {
        send_response(client_fd, 200, "application/json", GameAPI::SingletonGet()->GetStateJSON());
        return;
    }
    if (method == "GET" && (uri == "/screenshot" || uri == "/screenshot.png")) {
        std::string path = GameAPI::SingletonGet()->TakeScreenshot();
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) { send_response(client_fd, 404, "text/plain", "Screenshot not found"); return; }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string data(size, ' ');
        if (!file.read(&data[0], size)) { send_response(client_fd, 500, "text/plain", "Failed to read screenshot"); return; }
        send_response(client_fd, 200, "image/png", data);
        return;
    }
    if (method == "GET" && uri == "/screenshot_base64") {
        std::string path = GameAPI::SingletonGet()->TakeScreenshot();
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) { send_response(client_fd, 404, "text/plain", "Screenshot not found"); return; }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string data(size, ' ');
        if (!file.read(&data[0], size)) { send_response(client_fd, 500, "text/plain", "Failed to read screenshot"); return; }
        std::string resp = "{\"status\":\"ok\",\"image\":\"" + base64_encode(data) + "\"}";
        send_response(client_fd, 200, "application/json", resp);
        return;
    }
    if (method == "POST" && uri == "/action") {
        std::string key = extract_json_field(body, "key");
        std::string dur_str = extract_json_field(body, "duration_ms");
        int duration = 200;
        if (!dur_str.empty()) duration = std::atoi(dur_str.c_str());
        if (!key.empty()) InputInjector::SingletonGet()->QueueAction(key, duration);
        std::string resp = "{\"status\":\"ok\",\"action\":{\"key\":\"" + key + "\",\"duration_ms\":" + to_string(duration) + "}}";
        send_response(client_fd, 200, "application/json", resp);
        return;
    }
    if (method == "GET" && (uri == "/dashboard" || uri == "/dashboard/")) {
        send_response(client_fd, 200, "text/html", DASHBOARD_HTML);
        return;
    }
    send_response(client_fd, 404, "text/plain", "Not found: " + uri);
}

} // anonymous namespace

struct HTTPServerImpl {
    int listen_fd = -1;
    int port = 8080;
    std::atomic<bool> running{false};
    std::thread accept_thread;
};

HTTPServer::HTTPServer() : _impl(new HTTPServerImpl()), _ctx(nullptr), _port(0) {}
HTTPServer::~HTTPServer() { Stop(); delete _impl; }

HTTPServer* HTTPServer::SingletonGet() {
    static HTTPServer inst;
    return &inst;
}

bool HTTPServer::Start(int port) {
    auto* impl = static_cast<HTTPServerImpl*>(_ctx);
    if (impl && impl->running) return true;
    if (!impl) { _ctx = new HTTPServerImpl(); impl = static_cast<HTTPServerImpl*>(_ctx); }
    impl->port = port;
    impl->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (impl->listen_fd < 0) return false;
    int opt = 1;
    setsockopt(impl->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(impl->listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(impl->listen_fd); impl->listen_fd = -1; return false; }
    if (listen(impl->listen_fd, 5) < 0) { close(impl->listen_fd); impl->listen_fd = -1; return false; }
    set_nonblock(impl->listen_fd);
    impl->running = true;
    _port = port;
    impl->accept_thread = std::thread([impl]() {
        while (impl->running) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(impl->listen_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) { usleep(10000); continue; }
            set_nonblock(client_fd);
            char buf[4096];
            int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
            if (n <= 0) { close(client_fd); continue; }
            buf[n] = '\0';
            std::string request(buf);
            std::string method, uri;
            size_t sp = request.find(' ');
            if (sp != std::string::npos) {
                method = request.substr(0, sp);
                size_t sp2 = request.find(' ', sp + 1);
                if (sp2 != std::string::npos) uri = request.substr(sp + 1, sp2 - sp - 1);
                else uri = request.substr(sp + 1);
            }
            size_t hdr_end = request.find("\r\n\r\n");
            std::string body;
            if (hdr_end != std::string::npos) body = request.substr(hdr_end + 4);
            if (!method.empty() && !uri.empty()) handle_request(client_fd, method, uri, body);
            close(client_fd);
        }
    });
    return true;
}

void HTTPServer::Stop() {
    auto* impl = static_cast<HTTPServerImpl*>(_ctx);
    if (!impl) return;
    impl->running = false;
    if (impl->listen_fd >= 0) close(impl->listen_fd);
    if (impl->accept_thread.joinable()) impl->accept_thread.join();
    impl->listen_fd = -1;
}

} // namespace vt_mod
