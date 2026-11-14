#pragma once

#include <httplib.h>
#include <thread>
#include <mutex>

class GSIServer {
    friend struct GSIServerTestAccess;

  private:
    int port = 23561;
    class HttpServer final : public httplib::Server {
        bool process_and_close_socket(socket_t socket) override;
    } server;
    std::mutex lifecycleMutex, payloadMutex;
    std::thread worker;

    void gsiServer();

  public:
    ~GSIServer();
    bool run();
    void stop();
};
