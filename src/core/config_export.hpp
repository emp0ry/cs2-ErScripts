#pragma once

class GameConsole;
#include "game/cs2_config.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

class ConfigExport {
    friend struct ConfigExportTestAccess;

  public:
    struct Status {
        bool busy = false, queued = false, capturing = false, ready = false;
        std::string message;
        std::filesystem::path folder;
        size_t convars = 0, binds = 0, aliases = 0, files = 0;
    };
    static ConfigExport& instance();
    ~ConfigExport();
    Status status();
    void start(bool live, const std::filesystem::path& source = {});
    void cancel();
    void flush(GameConsole& console);
    void consume(std::wstring_view line);

  private:
    // Worker ownership is serialized separately from export data and filesystem I/O.
    std::mutex workerMutex_;
    std::mutex mutex_;
    std::jthread worker_;
    Status state_;
    cs2config::Snapshot snapshot_;
    std::string capture_, token_;
    std::uint64_t generation_ = 0;
    unsigned section_ = 0;
    size_t liveRows_ = 0;
    bool writing_ = false;
    std::chrono::steady_clock::time_point deadline_{};
    void prepare(std::stop_token stop, bool live, const std::filesystem::path& source);
    void finish(std::unique_lock<std::mutex>& lock);
};
