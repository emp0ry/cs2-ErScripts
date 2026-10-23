#include "FileMonitor.h"
#include "Logger.h"
#include <windows.h>
#include <filesystem>
#include <array>
#include <chrono>

FileMonitor::FileMonitor(std::wstring filePath) : filePath_(std::move(filePath)) {
    std::error_code error;
    const auto size = std::filesystem::file_size(filePath_, error);
    if (!error)
        lastOffset_ = static_cast<std::streamoff>(size);
}
FileMonitor::~FileMonitor() {
    stop();
}
void FileMonitor::start() {
    if (!running_.exchange(true))
        monitorThread_ = std::thread(&FileMonitor::pollFile, this);
}
void FileMonitor::stop() {
    running_ = false;
    if (monitorThread_.joinable())
        monitorThread_.join();
}
std::optional<std::deque<std::wstring>> FileMonitor::getNewLines() {
    std::lock_guard lock(linesMutex_);
    if (newLines_.empty())
        return std::nullopt;
    auto result = std::move(newLines_);
    newLines_.clear();
    return result;
}
void FileMonitor::pollFile() {
    std::string pending;
    while (running_) {
        std::ifstream file(std::filesystem::path(filePath_), std::ios::binary);
        if (!file) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        file.seekg(0, std::ios::end);
        if (file.tellg() < lastOffset_) {
            lastOffset_ = 0;
            pending.clear();
        }
        file.seekg(lastOffset_);
        std::array<char, 16384> buffer;
        while (file.read(buffer.data(), buffer.size()) || file.gcount()) {
            pending.append(buffer.data(), static_cast<size_t>(file.gcount()));
            lastOffset_ += file.gcount();
        }
        std::deque<std::wstring> lines;
        size_t begin = 0, end;
        while ((end = pending.find('\n', begin)) != std::string::npos) {
            auto line = pending.substr(begin, end - begin);
            begin = end + 1;
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty())
                continue;
            const int length = MultiByteToWideChar(CP_UTF8, 0, line.data(), static_cast<int>(line.size()), nullptr, 0);
            std::wstring wide(length, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, line.data(), static_cast<int>(line.size()), wide.data(), length);
            lines.push_back(std::move(wide));
        }
        pending.erase(0, begin);
        if (!lines.empty()) {
            std::lock_guard lock(linesMutex_);
            newLines_.insert(newLines_.end(), std::make_move_iterator(lines.begin()),
                             std::make_move_iterator(lines.end()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
