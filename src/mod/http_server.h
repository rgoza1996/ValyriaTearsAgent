// ValyriaTear HTTP API Mod — HTTP Server Header (POSIX sockets)

#ifndef VALYRIA_HTTP_SERVER_H
#define VALYRIA_HTTP_SERVER_H

#include <string>
#include <thread>
#include <atomic>

namespace vt_mod {

class HTTPServer {
public:
    static HTTPServer* SingletonGet();

    bool Start(int port = 8080);
    void Stop();
    bool IsRunning() const;

private:
    HTTPServer();
    ~HTTPServer();

    int server_fd_;
    std::thread accept_thread_;
    std::atomic<bool> running_;
    int _port;

    void accept_loop();
    void handle_client(int client_fd);
};

} // namespace vt_mod

#endif // VALYRIA_HTTP_SERVER_H