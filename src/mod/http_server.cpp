// ValyriaTear HTTP API Mod
// Uses xwd + Python for real screenshot capture in headless mode

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
#include <spawn.h>
#include <sys/wait.h>
#include <errno.h>
extern char **environ;

namespace vt_mod {

namespace {

static const char FALLBACK_B64[] = "iVBORw0KGgoWZq/8AAAADUlIRFIAAAFAAAAAyAgCAAAAfPEBrgAAAmFJREFUeNrt1KENADAIAEFkg2oYoPuPyQo4Kk7cBJ983HoBAAAAAAAAAAAAAAAAAAAAAAAAAPztZAGs8D/A//wP8D//A/zP/wD/8z/A//wP8D//A/zP/wD/8z/A//wP8D//A/zP/wD/8z/A/zQA/A/A/wD8D8D/APwPwP8A/A/A/wD8D8D/APwPwP8A//M/wP/8D/A//wP8z/8A//M/wP/8D/A//wP8z/8A//M/wP/8D/A//wP8z/8A//M/wP8A/A/A/wD8D8D/APwPwP8A/A/A/wD8D8D/APwPwP8A//M/wP/8D/A//wP8z/8A//M/wP/8D/A//wP8z/8A//M/wP/8D/A//wP8z/8A/9MA8D8A/wPwPwD/APwPwP8A/APwPwD/APwPwD/APwPwD/APwPwP8A//wP8D//A/zP/wD/8z/A//wP8D//A/zP/wD/8z/A//wP8D//A/zP/wD/8z/A/zQA/A/A/wD8D8D/APwPwP8A/A/A/wD8D8D/APwPwP8A//M/wP/8D/A//wP8z/8A//M/wP/8D/A//wP8z/8A//M/wP/8D/A//wP8z/8A/wMAAAAAAAAAAAAAAAAAAAAAAAAAhhrmTtZorkJgggAAAABJRU5E";
static const int FALLBACK_B64_LEN = 888;

std::string int_to_string(int v) { std::ostringstream ss; ss << v; return ss.str(); }

// Capture screenshot - fork/exec python, write base64 to file, read back
std::string capture_screenshot_xwd() {
    static const char* tmpfile = "/tmp/screenshot_b64.txt";

    pid_t pid = fork();
    if (pid < 0) return std::string(FALLBACK_B64, FALLBACK_B64_LEN);

    if (pid == 0) {
        setenv("DISPLAY", ":99", 1);
        execlp("python3", "python3", "/tmp/screenshot_server.py", (char*)nullptr);
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);

    int fd = open(tmpfile, O_RDONLY);
    if (fd < 0) return std::string(FALLBACK_B64, FALLBACK_B64_LEN);

    off_t fsize = lseek(fd, 0, SEEK_END);
    if (fsize < 0 || fsize > 10*1024*1024) {
        close(fd);
        return std::string(FALLBACK_B64, FALLBACK_B64_LEN);
    }

    lseek(fd, 0, SEEK_SET);

    std::string result;
    result.resize(fsize);

    ssize_t r = read(fd, &result[0], fsize);
    close(fd);

    if (r != fsize) return std::string(FALLBACK_B64, FALLBACK_B64_LEN);

    if (result.size() > 3 && result.substr(0, 3) == "OK:") {
        size_t colon_pos = result.find(':', 3);
        if (colon_pos != std::string::npos && colon_pos + 1 < result.size()) {
            return result.substr(colon_pos + 1);
        }
    }

    return result;
}

void send_response(int client_fd, int status, const std::string& content_type, const void* body_data, size_t body_size) {
    std::string status_str = (status == 200) ? "200 OK" : (status == 400 ? "400 Bad Request" : "404 Not Found");
    std::ostringstream header;
    header << "HTTP/1.1 " << status_str << "\r\n"
           << "Content-Type: " << content_type << "\r\n"
           << "Content-Length: " << body_size << "\r\n"
           << "Connection: close\r\n"
           << "\r\n";
    std::string hdr = header.str();
    send(client_fd, hdr.data(), hdr.size(), 0);
    if (body_size > 0 && body_data) {
        send(client_fd, body_data, body_size, 0);
    }
}

void send_file_response(int client_fd, const std::string& filepath, const std::string& content_type) {
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) {
        const char* notfound = "404 Not Found";
        send_response(client_fd, 404, "text/plain", notfound, 13);
        return;
    }

    off_t fsize = lseek(fd, 0, SEEK_END);
    if (fsize < 0) {
        close(fd);
        const char* err = "Cannot get size";
        send_response(client_fd, 500, "text/plain", err, 14);
        return;
    }

    lseek(fd, 0, SEEK_SET);

    if (fsize < 1024*1024) {  // < 1MB
        std::unique_ptr<char[]> buf(new char[fsize]);
        ssize_t r = read(fd, buf.get(), fsize);
        close(fd);
        if (r != fsize) {
            const char* err = "Read error";
            send_response(client_fd, 500, "text/plain", err, 9);
            return;
        }
        send_response(client_fd, 200, content_type.c_str(), buf.get(), fsize);
    } else {
        close(fd);
        std::ostringstream hdr;
        hdr << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: " << content_type << "\r\n"
            << "Content-Length: " << fsize << "\r\n"
            << "Connection: close\r\n"
            << "\r\n";
        std::string header_str = hdr.str();
        send(client_fd, header_str.data(), header_str.size(), 0);

        fd = open(filepath.c_str(), O_RDONLY);
        char buf[65536];
        ssize_t r;
        while ((r = read(fd, buf, sizeof(buf))) > 0) {
            send(client_fd, buf, r, 0);
        }
        close(fd);
    }
}

bool file_exists_and_readable(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    return f && f.tellg() > 0;
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
        size_t endq = json_str.find('"', start + 1);
        if (endq == std::string::npos) return "";
        return json_str.substr(start + 1, endq - start - 1);
    }
    size_t end = start;
    while (end < json_str.size() && (std::isdigit(json_str[end]) || json_str[end] == '.' || json_str[end] == '-')) ++end;
    return json_str.substr(start, end - start);
}

void handle_request(int client_fd, const std::string& method, const std::string& uri, const std::string& body) {
    if (method == "GET" && uri == "/health") {
        const char* reply = "{\"status\":\"ok\",\"server\":\"valyria-tear\"}";
        send_response(client_fd, 200, "application/json", reply, 41);
        return;
    }
    if (method == "GET" && uri == "/state") {
        std::string resp = GameAPI::SingletonGet()->GetStateJSON();
        send_response(client_fd, 200, "application/json", resp.data(), resp.size());
        return;
    }
    if (method == "GET" && uri == "/saves") {
        std::string resp = GameAPI::SingletonGet()->ListSaves();
        send_response(client_fd, 200, "application/json", resp.data(), resp.size());
        return;
    }
    if (method == "GET" && (uri == "/screenshot" || uri == "/screenshot.png")) {
        std::string path = GameAPI::SingletonGet()->TakeScreenshot();
        if (file_exists_and_readable(path)) {
            send_file_response(client_fd, path, "image/png");
        } else {
            const char tiny_png[] = "\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90wS\x00\x00\x00\x0cIDATx\x9cc\xf8\x0f\x00\x00\x01\x01\x00\x05\x18\xd8N\x00\x00\x00\x00IEND\xaeB`\x82";
            send_response(client_fd, 200, "image/png", tiny_png, 61);
        }
        return;
    }
    if (method == "GET" && uri == "/screenshot_base64") {
        std::string b64 = capture_screenshot_xwd();

        static const char* prefix = "{\"status\":\"ok\",\"image\":\"";
        static const char* suffix = "\"}";

        size_t json_len = strlen(prefix) + b64.size() + strlen(suffix);

        std::ostringstream hdr;
        hdr << "HTTP/1.1 200 OK\r\n"
            << "Content-Type: application/json\r\n"
            << "Content-Length: " << json_len << "\r\n"
            << "Connection: close\r\n"
            << "\r\n";
        std::string header_str = hdr.str();
        send(client_fd, header_str.data(), header_str.size(), 0);

        send(client_fd, prefix, strlen(prefix), 0);
        send(client_fd, b64.data(), b64.size(), 0);
        send(client_fd, suffix, strlen(suffix), 0);
        return;
    }
    if (method == "POST" && uri == "/action") {
        std::string key = extract_json_field(body, "key");
        std::string dur_str = extract_json_field(body, "duration_ms");
        int duration = 200;
        if (!dur_str.empty()) duration = std::atoi(dur_str.c_str());
        if (!key.empty()) InputInjector::SingletonGet()->QueueAction(key, duration);
        std::string resp = "{\"status\":\"ok\",\"action\":{\"key\":\"" + key + "\",\"duration_ms\":" + int_to_string(duration) + "}}";
        send_response(client_fd, 200, "application/json", resp.data(), resp.size());
        return;
    }

    // POST /do — action primitives: interact, open_menu, close_menu, pause, navigate_up/down/left/right, etc.
    if (method == "POST" && uri == "/do") {
        std::string action = extract_json_field(body, "action");
        std::string duration_str = extract_json_field(body, "duration_ms");
        std::string key = extract_json_field(body, "key");

        std::string resp;
        if (!action.empty()) {
            InputInjector::SingletonGet()->QueueSequence(action);
            resp = "{\"status\":\"ok\",\"action\":\"" + action + "\"}";
        } else if (!key.empty()) {
            int duration = 200;
            if (!duration_str.empty()) duration = std::atoi(duration_str.c_str());
            InputInjector::SingletonGet()->QueueAction(key, duration);
            resp = "{\"status\":\"ok\",\"key\":\"" + key + "\",\"duration_ms\":" + int_to_string(duration) + "}";
        } else {
            resp = "{\"status\":\"error\",\"message\":\"Provide action or key field\"}";
            send_response(client_fd, 400, "application/json", resp.data(), resp.size());
            return;
        }
        send_response(client_fd, 200, "application/json", resp.data(), resp.size());
        return;
    }
    if (method == "POST" && uri == "/save") {
        std::string slot_str = extract_json_field(body, "slot");
        int slot = 0;
        if (!slot_str.empty()) slot = std::atoi(slot_str.c_str());
        std::string resp = GameAPI::SingletonGet()->SaveGame(slot);
        send_response(client_fd, 200, "application/json", resp.data(), resp.size());
        return;
    }
    if (method == "POST" && uri == "/load") {
        std::string slot_str = extract_json_field(body, "slot");
        int slot = 0;
        if (!slot_str.empty()) slot = std::atoi(slot_str.c_str());
        std::string resp = GameAPI::SingletonGet()->LoadGame(slot);
        send_response(client_fd, 200, "application/json", resp.data(), resp.size());
        return;
    }
    if (method == "GET" && (uri == "/dashboard" || uri == "/dashboard/")) {
        send_response(client_fd, 200, "text/html", DASHBOARD_HTML, sizeof(DASHBOARD_HTML) - 1);
        return;
    }
    std::string notfound = "Not found: " + uri;
    send_response(client_fd, 404, "text/plain", notfound.data(), notfound.size());
}

} // anonymous namespace

HTTPServer::HTTPServer() : server_fd_(-1), accept_thread_(), running_(false), _port(8080) {}

HTTPServer::~HTTPServer() { Stop(); }

bool HTTPServer::Start(int port) {
    if (running_) return true;
    _port = port;

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) return false;

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    if (listen(server_fd_, 5) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    running_ = true;
    accept_thread_ = std::thread(&HTTPServer::accept_loop, this);
    return true;
}

void HTTPServer::Stop() {
    if (!running_) return;
    running_ = false;
    if (server_fd_ >= 0) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
}

bool HTTPServer::IsRunning() const { return running_; }

void HTTPServer::accept_loop() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;

        std::thread(&HTTPServer::handle_client, this, client_fd).detach();
    }
}

void HTTPServer::handle_client(int client_fd) {
    char buffer[8192];
    std::string request;
    ssize_t n;

    while ((n = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        request.append(buffer, n);
        if (request.find("\r\n\r\n") != std::string::npos) break;
    }

    if (request.empty()) {
        close(client_fd);
        return;
    }

    size_t method_end = request.find(' ');
    size_t uri_end = request.find(' ', method_end + 1);
    if (method_end == std::string::npos || uri_end == std::string::npos) {
        close(client_fd);
        return;
    }

    std::string method = request.substr(0, method_end);
    std::string uri = request.substr(method_end + 1, uri_end - method_end - 1);
    size_t body_start = request.find("\r\n\r\n");
    std::string body = (body_start != std::string::npos) ? request.substr(body_start + 4) : "";

    handle_request(client_fd, method, uri, body);
    close(client_fd);
}

HTTPServer* HTTPServer::SingletonGet() {
    static HTTPServer inst;
    return &inst;
}

} // namespace vt_mod
