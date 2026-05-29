////////////////////////////////////////////////////////////////////////////////
// ValyriaTear HTTP API Mod — HTTP Server (civetweb)
////////////////////////////////////////////////////////////////////////////////

#ifndef VALYRIA_HTTP_SERVER_H
#define VALYRIA_HTTP_SERVER_H

#include <string>

struct mg_context;
struct mg_connection;

namespace vt_mod {

class HTTPServer {
public:
    static HTTPServer* SingletonGet();

    bool Start(int port = 8080);
    void Stop();
    bool IsRunning() const { return _ctx != nullptr; }
    int GetPort() const { return _port; }

private:
    HTTPServer();
    ~HTTPServer();

    static int _HandleRequest(::mg_connection* conn, void* cbdata);

    struct mg_context* _ctx;
    int _port;
};

} // namespace vt_mod

#endif // VALYRIA_HTTP_SERVER_H