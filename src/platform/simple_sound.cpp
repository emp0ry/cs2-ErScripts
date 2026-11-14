#include "platform/simple_sound.hpp"

#include "core/logger.hpp"

#include <atomic>
#include <fstream>
#include <mmeapi.h>
#include <mmsystem.h>
#include <windows.h>

#pragma comment(lib, "winmm.lib")

namespace fs = std::filesystem;

SimpleSound::SimpleSound(const unsigned char* data, unsigned int size) {
    if (!loadFromBuffer(tempDefaultFile_, data, size)) {
        Logger::logWarning("Failed to load sound from buffer (size: " + std::to_string(size) + ")");
        throw std::runtime_error("Failed to load sound from buffer");
    }
}

SimpleSound::~SimpleSound() {
    stop_.request_stop();
    for (auto& request : playRequests_) {
        try {
            request.get();
        } catch (const std::exception& error) {
            Logger::logWarning(std::format("Sound playback failed: {}", error.what()));
        }
    }
    if (!tempDefaultFile_.empty()) {
        removeTempFile(tempDefaultFile_);
    }
}

void SimpleSound::setVolume(int volume) noexcept {
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;
    volume_ = volume / 100.0f;
    DWORD vol = static_cast<DWORD>(volume_ * 0xFFFF);
    waveOutSetVolume(nullptr, (vol << 16) | vol);
}

void SimpleSound::play() {
    playRequests_.remove_if([](std::future<void>& request) {
        if (request.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return false;
        try {
            request.get();
        } catch (const std::exception& error) {
            Logger::logWarning(std::format("Sound playback failed: {}", error.what()));
        }
        return true;
    });
    // Long custom WAV files must not create an unbounded number of playback threads.
    if (std::distance(playRequests_.begin(), playRequests_.end()) >= 16)
        return;
    std::wstring filePath = tempSecondFile_.empty() ? tempDefaultFile_ : tempSecondFile_;

    playRequests_.emplace_front(
        std::async(std::launch::async, [filePath, tempDefaultFile = tempDefaultFile_, stop = stop_.get_token()]() {
            std::wstring alias = generateAlias();
            struct Playback {
                std::wstring close;
                bool opened = false;
                ~Playback() {
                    if (opened)
                        mciSendStringW(close.c_str(), nullptr, 0, nullptr);
                }
            } playback{L"close " + alias};
            const auto status = L"status " + alias + L" mode";
            if (stop.stop_requested())
                return;

            auto error = mciSendStringW((L"open \"" + filePath + L"\" type waveaudio alias " + alias).c_str(), nullptr,
                                        0, nullptr);
            if (error != 0) {
                error = mciSendStringW((L"open \"" + tempDefaultFile + L"\" type waveaudio alias " + alias).c_str(),
                                       nullptr, 0, nullptr);
                if (error != 0) {
                    Logger::logWarning(std::format("Cannot open WAV file (MCI error {})", error));
                    return;
                }
            }
            playback.opened = true;

            if (!stop.stop_requested() && mciSendStringW((L"play " + alias).c_str(), nullptr, 0, nullptr) != 0) {
                Logger::logWarning("Failed to play WAV");
            } else {
                while (!stop.stop_requested()) {
                    wchar_t mode[32]{};
                    if (mciSendStringW(status.c_str(), mode, static_cast<UINT>(std::size(mode)), nullptr) != 0) {
                        Logger::logWarning("Cannot read WAV playback status");
                        break;
                    }
                    if (std::wstring_view(mode) != L"playing")
                        break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }));
}

bool SimpleSound::secondBuffer(const std::string& filePath) {
    tempSecondFile_.clear();
    if (filePath.empty())
        return false;
    auto path = fs::path(std::u8string(filePath.begin(), filePath.end()));
    std::error_code error;
    if (!fs::is_regular_file(path, error)) {
        path += L".WAV";
        if (!fs::is_regular_file(path, error))
            return false;
    }
    tempSecondFile_ = path.wstring();
    return true;
}

bool SimpleSound::loadFromBuffer(std::wstring& tempFile, const unsigned char* data, unsigned int size) {
    if (!data || size == 0) {
        Logger::logWarning("Invalid buffer: null or zero size");
        return false;
    }

    tempFile = createTempFile(data, size);
    return !tempFile.empty();
}

std::wstring SimpleSound::createTempFile(const unsigned char* data, unsigned int size) {
    wchar_t tempPath[MAX_PATH];
    wchar_t tempFile[MAX_PATH];
    const DWORD length = GetTempPathW(MAX_PATH, tempPath);
    if (!length || length >= MAX_PATH || GetTempFileNameW(tempPath, L"WAV", 0, tempFile) == 0) {
        Logger::logWarning("Failed to create temporary file path");
        return L"";
    }

    std::ofstream file(tempFile, std::ios::binary);
    if (!file.is_open()) {
        Logger::logWarning("Failed to open temporary file for writing");
        DeleteFileW(tempFile);
        return L"";
    }

    file.write(reinterpret_cast<const char*>(data), size);
    file.close();
    if (!file) {
        Logger::logWarning("Failed to write to temporary file");
        DeleteFileW(tempFile);
        return L"";
    }

    return std::wstring(tempFile);
}
bool SimpleSound::removeTempFile(std::wstring& tempFile) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    std::error_code error;
    do {
        fs::remove(tempFile, error);
        if (!error) {
            tempFile.clear();
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (std::chrono::steady_clock::now() < deadline);
    Logger::logWarning(std::format("Cannot remove temporary sound: {}", error.message()));
    return false;
}

std::wstring SimpleSound::generateAlias() {
    static std::atomic_uint64_t next{0};
    return L"ers_sound_" + std::to_wstring(next.fetch_add(1, std::memory_order_relaxed));
}
