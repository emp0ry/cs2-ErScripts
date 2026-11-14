#pragma once
#include "CS2Config.h"
#include <chrono>
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
    static ConfigExport &instance();
    ~ConfigExport();
    Status status();
    // The render thread owns start/cancel; status, flush and consume may run on other threads.
    void start(bool live, const std::filesystem::path &source = {});
    void cancel();
    void flush();
    void consume(std::wstring_view line);

  private:
    std::mutex mutex;
    std::jthread worker;
    Status state;
    cs2config::Snapshot snapshot;
    std::string capture, token;
    unsigned section = 0;
    size_t liveRows = 0;
    bool writing = false;
    std::chrono::steady_clock::time_point deadline{};
    void prepare(std::stop_token stop, bool live, const std::filesystem::path &source);
    void finish(std::unique_lock<std::mutex> &lock);
};
