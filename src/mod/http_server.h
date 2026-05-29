////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — HTTP Server (civetweb)
// Listens on port 8080, exposes REST endpoints for bot control
////////////////////////////////////////////////////////////////////////////////

#ifndef VALYRIA_HTTP_SERVER_H
#define VALYRIA_HTTP_SERVER_H

#include <string>

struct mg_context;

namespace vt_mod {

class HTTPServer {
public:
    static HTTPServer* SingletonGet();

    // Start the server on given port (default 8080)
    bool Start(int port = 8080);

    // Stop the server
    void Stop();

    // Check if running
    bool IsRunning() const { return _ctx != nullptr; }

    int GetPort() const { return _port; }

    // Called from main.cpp to update input injector each frame
    static void UpdateInput();

private:
    HTTPServer();
    ~HTTPServer();

    static int _HandleRequest(struct mg_connection* conn, void* cbdata);

    struct mg_context* _ctx;
    int _port;
};

} // namespace vt_mod

#endif // VALYRIA_HTTP_SERVER_H