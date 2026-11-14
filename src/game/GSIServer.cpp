#include "game/GSIServer.h"
#include "core/Logger.h"
#include "core/Globals.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <system_error>

namespace {
    void CALLBACK expireRequest(PTP_CALLBACK_INSTANCE, void *context, PTP_TIMER) noexcept {
        ::shutdown(*static_cast<socket_t *>(context), SD_BOTH);
    }
} // namespace

bool GSIServer::HttpServer::process_and_close_socket(socket_t socket) {
    struct Connection {
        socket_t socket;
        ~Connection() {
            ::shutdown(socket, SD_BOTH);
            ::closesocket(socket);
        }
    } connection{socket};
    if (svr_sock_.load() == INVALID_SOCKET)
        return false;
    try {
        std::string remote, local;
        int remotePort = 0, localPort = 0;
        httplib::detail::get_remote_ip_and_port(socket, remote, remotePort);
        httplib::detail::get_local_ip_and_port(socket, local, localPort);
        return httplib::detail::process_server_socket_core(
            svr_sock_, socket, keep_alive_max_count_, keep_alive_timeout_sec_, [&](bool close, bool &closed) {
                if (svr_sock_.load() == INVALID_SOCKET)
                    return false;
                // A socket shutdown also interrupts partial headers that outlive individual read timeouts.
                const auto closeTimer = [](PTP_TIMER timer) {
                    SetThreadpoolTimer(timer, nullptr, 0, 0);
                    // Drain callbacks before the accepted socket can be closed and its handle reused.
                    WaitForThreadpoolTimerCallbacks(timer, TRUE);
                    CloseThreadpoolTimer(timer);
                };
                std::unique_ptr<std::remove_pointer_t<PTP_TIMER>, decltype(closeTimer)> timer(
                    CreateThreadpoolTimer(expireRequest, &socket, nullptr), closeTimer);
                if (!timer)
                    throw std::system_error(GetLastError(), std::system_category(), "Cannot limit GSI request time");
                // A negative FILETIME is a relative deadline in 100 ns units.
                ULARGE_INTEGER relative{};
                relative.QuadPart = static_cast<ULONGLONG>(-20'000'000LL);
                FILETIME due{relative.LowPart, relative.HighPart};
                SetThreadpoolTimer(timer.get(), &due, 0, 0);
                httplib::detail::SocketStream stream(socket, read_timeout_sec_, read_timeout_usec_, write_timeout_sec_,
                                                     write_timeout_usec_);
                return process_request(stream, remote, remotePort, local, localPort, close, closed, nullptr);
            });
    } catch (const std::exception &error) {
        Logger::logWarning(std::format("GSI connection stopped: {}", error.what()));
        return false;
    }
}

void GSIServer::gsiServer() {
    server.Post("/", [this](const httplib::Request &req, httplib::Response &res) {
        try {

            {
                std::lock_guard lock(payloadMutex);
                const auto jsonData = nlohmann::json::parse(req.body);
                globals::updateGameState(jsonData);
            }
            res.set_content("OK", "text/plain");
        } catch (const nlohmann::json::exception &e) {
            Logger::logWarning(std::format("JSON parsing error: {}", e.what()));
            res.status = 400;
            res.set_content("Invalid JSON", "text/plain");
        }
    });

    // Idle keep-alive connections must not block other clients; only packet publication is serialized.
    server.new_task_queue = [] { return new httplib::ThreadPool(4, 64); };
    server.set_read_timeout(2, 0);
    server.set_write_timeout(2, 0);
    server.set_payload_max_length(1024 * 1024);
    bool bound;
    if (port == 0) {
        port = server.bind_to_any_port("127.0.0.1");
        bound = port > 0;
    } else {
        bound = server.bind_to_port("127.0.0.1", port);
    }
    if (!bound) {
        Logger::logWarning(std::format("Failed to start GSI server on 127.0.0.1:{}", port));
        return;
    }
    Logger::logInfo(std::format("Starting GSI server on 127.0.0.1:{}", port));
    if (!server.listen_after_bind())
        Logger::logWarning("GSI server stopped after a listener error");
}

bool GSIServer::run() {
    std::lock_guard owner(lifecycleMutex);
    if (worker.joinable())
        return server.is_running();
    worker = std::thread([this] {
        try {
            gsiServer();
        } catch (const std::exception &error) {
            server.stop();
            server.decommission();
            Logger::logWarning(std::format("GSI server stopped: {}", error.what()));
        }
    });
    // stop() must not race with listen() before cpp-httplib marks the server running.
    server.wait_until_ready();
    return server.is_running();
}

GSIServer::~GSIServer() {
    stop();
}

void GSIServer::stop() {
    std::lock_guard owner(lifecycleMutex);
    if (!worker.joinable())
        return;
    Logger::logInfo("Stopping GSI server");
    server.stop();
    worker.join();
}
