#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

class FileMonitor {
  public:
    explicit FileMonitor(std::wstring filePath);
    ~FileMonitor();
    FileMonitor(const FileMonitor &) = delete;
    FileMonitor &operator=(const FileMonitor &) = delete;

    void start();
    void stop();
    [[nodiscard]] std::optional<std::deque<std::wstring>> getNewLines();

  private:
    std::wstring filePath_;
    std::mutex workerMutex_, waitMutex_, linesMutex_;
    std::condition_variable_any wake_;
    std::jthread worker_;
    std::atomic_bool running_ = false;
    std::deque<std::wstring> newLines_;
    size_t queuedChars_ = 0;
    bool queueOverflow_ = false, droppingLine_ = false;
    std::uint64_t lastOffset_ = 0;
    std::optional<std::array<std::uint32_t, 3>> identity_;
    std::string pending_;
    std::array<char, 64> tail_{};
    size_t tailSize_ = 0;

    void pollFile(std::stop_token stop);
    void append(std::string_view bytes);
};
