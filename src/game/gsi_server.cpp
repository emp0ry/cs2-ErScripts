#include "game/gsi_server.hpp"

#include "core/globals.hpp"
#include "core/logger.hpp"

#include <memory>
#include <system_error>

#include <nlohmann/json.hpp>

namespace {
    void CALLBACK expireRequest(PTP_CALLBACK_INSTANCE, void* context, PTP_TIMER) noexcept {
        ::shutdown(*static_cast<socket_t*>(context), SD_BOTH);
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
            svr_sock_, socket, keep_alive_max_count_, keep_alive_timeout_sec_, [&](bool close, bool& closed) {
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
    } catch (const std::exception& error) {
        Logger::logWarning(std::format("GSI connection stopped: {}", error.what()));
        return false;
    }
}

void GSIServer::gsiServer() {
    server_.Post("/", [this](const httplib::Request& req, httplib::Response& res) {
        try {

            {
                std::lock_guard lock(payloadMutex_);
                const auto jsonData = nlohmann::json::parse(req.body);
                globals::updateGameState(jsonData);
            }
            res.set_content("OK", "text/plain");
        } catch (const nlohmann::json::exception& e) {
            Logger::logWarning(std::format("JSON parsing error: {}", e.what()));
            res.status = 400;
            res.set_content("Invalid JSON", "text/plain");
        }
    });

    // Idle keep-alive connections must not block other clients; only packet publication is serialized.
    server_.new_task_queue = [] { return new httplib::ThreadPool(4, 64); };
    server_.set_read_timeout(2, 0);
    server_.set_write_timeout(2, 0);
    server_.set_payload_max_length(1024 * 1024);
    bool bound;
    if (port_ == 0) {
        port_ = server_.bind_to_any_port("127.0.0.1");
        bound = port_ > 0;
    } else {
        bound = server_.bind_to_port("127.0.0.1", port_);
    }
    if (!bound) {
        Logger::logWarning(std::format("Failed to start GSI server on 127.0.0.1:{}", port_));
        return;
    }
    Logger::logInfo(std::format("Starting GSI server on 127.0.0.1:{}", port_));
    if (!server_.listen_after_bind())
        Logger::logWarning("GSI server stopped after a listener error");
}

bool GSIServer::run() {
    std::lock_guard owner(lifecycleMutex_);
    if (worker_.joinable())
        return server_.is_running();
    worker_ = std::thread([this] {
        try {
            gsiServer();
        } catch (const std::exception& error) {
            server_.stop();
            server_.decommission();
            Logger::logWarning(std::format("GSI server stopped: {}", error.what()));
        }
    });
    // stop() must not race with listen() before cpp-httplib marks the server running.
    server_.wait_until_ready();
    return server_.is_running();
}

GSIServer::~GSIServer() {
    stop();
}

void GSIServer::stop() {
    std::lock_guard owner(lifecycleMutex_);
    if (!worker_.joinable())
        return;
    Logger::logInfo("Stopping GSI server");
    server_.stop();
    worker_.join();
}
