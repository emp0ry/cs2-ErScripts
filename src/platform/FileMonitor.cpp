#include "platform/FileMonitor.h"
#include "core/Logger.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <memory>
#include <cstring>

namespace {
    constexpr size_t lineLimit = 1024 * 1024;
    using FileHandle = std::unique_ptr<void, decltype(&CloseHandle)>;
    FileHandle openLog(const std::wstring &path) {
        const auto handle =
            CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        return FileHandle(handle == INVALID_HANDLE_VALUE ? nullptr : handle, CloseHandle);
    }
    auto identity(const BY_HANDLE_FILE_INFORMATION &info) {
        return std::array<std::uint32_t, 3>{info.dwVolumeSerialNumber, info.nFileIndexHigh, info.nFileIndexLow};
    }
    std::uint64_t fileSize(const BY_HANDLE_FILE_INFORMATION &info) {
        return (static_cast<std::uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    }
} // namespace

FileMonitor::FileMonitor(std::wstring path) : filePath_(std::move(path)) {
    const auto file = openLog(filePath_);
    BY_HANDLE_FILE_INFORMATION info{};
    if (file && GetFileInformationByHandle(file.get(), &info)) {
        identity_ = identity(info);
        lastOffset_ = fileSize(info);
        tailSize_ = static_cast<size_t>(std::min<std::uint64_t>(tail_.size(), lastOffset_));
        LARGE_INTEGER offset{};
        offset.QuadPart = static_cast<LONGLONG>(lastOffset_ - tailSize_);
        DWORD bytes = 0;
        if (!SetFilePointerEx(file.get(), offset, nullptr, FILE_BEGIN) ||
            !ReadFile(file.get(), tail_.data(), static_cast<DWORD>(tailSize_), &bytes, nullptr) || bytes != tailSize_)
            tailSize_ = 0;
        // Monitoring starts at the old EOF; its unfinished line must not become a new event.
        droppingLine_ = tailSize_ && tail_[tailSize_ - 1] != '\n';
    }
}
FileMonitor::~FileMonitor() {
    stop();
}

void FileMonitor::start() {
    std::lock_guard owner(workerMutex_);
    if (running_)
        return;
    if (worker_.joinable())
        worker_.join();
    running_ = true;
    try {
        worker_ = std::jthread([this](std::stop_token stop) {
            try {
                pollFile(stop);
            } catch (const std::exception &error) {
                Logger::logWarning(std::format("Console monitor stopped: {}", error.what()));
            }
            running_ = false;
        });
    } catch (...) {
        running_ = false;
        throw;
    }
}
void FileMonitor::stop() {
    std::lock_guard owner(workerMutex_);
    worker_.request_stop();
    if (worker_.joinable())
        worker_.join();
    running_ = false;
}
std::optional<std::deque<std::wstring>> FileMonitor::getNewLines() {
    std::lock_guard lock(linesMutex_);
    if (newLines_.empty())
        return std::nullopt;
    auto result = std::move(newLines_);
    newLines_.clear();
    queuedChars_ = 0;
    queueOverflow_ = false;
    return result;
}
void FileMonitor::append(std::string_view bytes) {
    const size_t retained = std::min(tailSize_, tail_.size() - std::min(bytes.size(), tail_.size()));
    std::memmove(tail_.data(), tail_.data() + tailSize_ - retained, retained);
    const auto suffix = bytes.substr(bytes.size() - std::min(bytes.size(), tail_.size()));
    std::memcpy(tail_.data() + retained, suffix.data(), suffix.size());
    tailSize_ = retained + suffix.size();
    while (!bytes.empty()) {
        const auto newline = bytes.find('\n');
        const auto part = bytes.substr(0, newline);
        if (!droppingLine_) {
            if (part.size() > lineLimit - pending_.size()) {
                pending_.clear();
                droppingLine_ = true;
                Logger::logWarning("Console line exceeded 1 MiB; skipping it");
            } else {
                pending_.append(part);
            }
        }
        if (newline == bytes.npos)
            return;
        bytes.remove_prefix(newline + 1);
        if (!droppingLine_ && !pending_.empty()) {
            if (pending_.back() == '\r')
                pending_.pop_back();
            const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pending_.data(),
                                                   static_cast<int>(pending_.size()), nullptr, 0);
            if (length > 0) {
                std::wstring line(length, L'\0');
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pending_.data(), static_cast<int>(pending_.size()),
                                    line.data(), length);
                std::lock_guard lock(linesMutex_);
                queuedChars_ += line.size();
                newLines_.push_back(std::move(line));
                bool dropped = false;
                while (newLines_.size() > 16384 || queuedChars_ > 4 * 1024 * 1024) {
                    queuedChars_ -= newLines_.front().size();
                    newLines_.pop_front();
                    dropped = true;
                }
                if (dropped && !queueOverflow_)
                    Logger::logWarning("Console queue exceeded its limit; discarded the oldest lines");
                queueOverflow_ |= dropped;
            }
        }
        pending_.clear();
        droppingLine_ = false;
    }
}
void FileMonitor::pollFile(std::stop_token stop) {
    std::array<char, 4096> buffer;
    bool readFailed = false;
    while (!stop.stop_requested()) {
        const auto file = openLog(filePath_);
        BY_HANDLE_FILE_INFORMATION info{};
        if (file && GetFileInformationByHandle(file.get(), &info)) {
            const auto current = identity(info);
            bool restarted = !identity_ || *identity_ != current || fileSize(info) < lastOffset_;
            if (!restarted && tailSize_) {
                // File identity and length alone miss truncation followed by rapid regrowth.
                std::array<char, 64> previous{};
                LARGE_INTEGER position{};
                position.QuadPart = static_cast<LONGLONG>(lastOffset_ - tailSize_);
                DWORD bytes = 0;
                if (!SetFilePointerEx(file.get(), position, nullptr, FILE_BEGIN) ||
                    !ReadFile(file.get(), previous.data(), static_cast<DWORD>(tailSize_), &bytes, nullptr)) {
                    if (!readFailed)
                        Logger::logWarning(std::format("Cannot check console log continuity: {}", GetLastError()));
                    readFailed = true;
                    std::unique_lock wait(waitMutex_);
                    wake_.wait_for(wait, stop, std::chrono::milliseconds(50), [] { return false; });
                    continue;
                }
                restarted = bytes != tailSize_ || std::memcmp(previous.data(), tail_.data(), tailSize_) != 0;
            }
            if (restarted) {
                lastOffset_ = 0;
                tailSize_ = 0;
                pending_.clear();
                droppingLine_ = false;
            }
            identity_ = current;
            LARGE_INTEGER offset{};
            offset.QuadPart = static_cast<LONGLONG>(lastOffset_);
            bool ok = SetFilePointerEx(file.get(), offset, nullptr, FILE_BEGIN) != FALSE;
            size_t total = 0;
            while (ok && !stop.stop_requested() && total < 1024 * 1024) {
                DWORD bytes = 0;
                ok = ReadFile(file.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytes, nullptr) != FALSE;
                if (!ok || !bytes)
                    break;
                lastOffset_ += bytes;
                total += bytes;
                append(std::string_view(buffer.data(), bytes));
            }
            if (!ok && !readFailed)
                Logger::logWarning(std::format("Cannot read console log: {}", GetLastError()));
            readFailed = !ok;
        }
        std::unique_lock wait(waitMutex_);
        wake_.wait_for(wait, stop, std::chrono::milliseconds(50), [] { return false; });
    }
}
