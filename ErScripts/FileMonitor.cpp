#include "FileMonitor.h"
#include "Logger.h"
#include <iostream>
#include <chrono>

FileMonitor::FileMonitor(std::wstring filePath) : filePath_(std::move(filePath)) {
    // Start at the end of the file
    std::wifstream file(filePath_, std::ios::in);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        lastOffset_ = file.tellg();
        file.close();
    }
}

FileMonitor::~FileMonitor() {
    stop();
}

void FileMonitor::start() {
    if (running_.exchange(true)) {
        return;
    }
    monitorThread_ = std::thread(&FileMonitor::pollFile, this);
}

void FileMonitor::stop() {
    running_ = false;
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
}

std::optional<std::deque<std::wstring>> FileMonitor::getNewLines() {
    std::lock_guard<std::mutex> lock(linesMutex_);
    if (newLines_.empty()) {
        return std::nullopt;
    }
    auto result = std::move(newLines_);
    newLines_.clear();
    return result;
}

void FileMonitor::pollFile() {
    while (running_) {
        std::wifstream file(filePath_, std::ios::in);
        if (!file.is_open()) {
            Logger::logWarning(std::format(L"Failed to open CS2 log: {}", filePath_));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        // Move to last known offset
        file.seekg(lastOffset_, std::ios::beg);

        std::deque<std::wstring> tempLines;
        std::wstring line;

        // Read all new lines from current position
        while (std::getline(file, line)) {
            if (!line.empty()) {
                tempLines.push_back(std::move(line));
            }
        }

        // Update offset to current position
        lastOffset_ = file.tellg();
        if (lastOffset_ == -1) {  // Handle potential EOF or error
            file.clear();
            file.seekg(0, std::ios::end);
            lastOffset_ = file.tellg();
        }

        if (!tempLines.empty()) {
            std::lock_guard<std::mutex> lock(linesMutex_);
            newLines_.insert(newLines_.end(),
                std::move_iterator(tempLines.begin()),
                std::move_iterator(tempLines.end()));
        }

        file.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Fast but light
    }
}