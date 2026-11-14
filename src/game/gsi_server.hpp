#pragma once

#include <mutex>
#include <thread>

#include <httplib.h>

class GSIServer {
    friend struct GSIServerTestAccess;

  private:
    int port_ = 23561;
    class HttpServer final : public httplib::Server {
        bool process_and_close_socket(socket_t socket) override;
    } server_;
    std::mutex lifecycleMutex_, payloadMutex_;
    std::thread worker_;

    void gsiServer();

  public:
    ~GSIServer();
    bool run();
    void stop();
};
