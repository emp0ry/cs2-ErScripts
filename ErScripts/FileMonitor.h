#pragma once

#include <string>
#include <deque>
#include <optional>
#include <atomic>
#include <mutex>
#include <thread>
#include <fstream>

class FileMonitor {
public:
    explicit FileMonitor(std::wstring filePath);
    ~FileMonitor();

    FileMonitor(const FileMonitor&) = delete;
    FileMonitor& operator=(const FileMonitor&) = delete;

    void start();
    void stop();
    [[nodiscard]] std::optional<std::deque<std::wstring>> getNewLines();

private:
    std::wstring filePath_;
    std::atomic<bool> running_{ false };
    std::thread monitorThread_;
    std::mutex linesMutex_;
    std::deque<std::wstring> newLines_;
    std::streamoff lastOffset_{ 0 };

    void pollFile();
};