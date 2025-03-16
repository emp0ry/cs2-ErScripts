#include "SimpleSound.h"
#include "Logger.h"
#include <fstream>
#include <random>
#include <windows.h>
#include <mmeapi.h>

#pragma comment(lib, "winmm.lib")

namespace fs = std::filesystem;

SimpleSound::SimpleSound(const unsigned char* data, unsigned int size) {
    if (!loadFromBuffer(tempDefaultFile_, data, size)) {
        Logger::logWarning("Failed to load sound from buffer (size: " + std::to_string(size) + ")");
        throw std::runtime_error("Failed to load sound from buffer");
    }
}

SimpleSound::~SimpleSound() {
    if (!tempDefaultFile_.empty()) {
        removeTempFile(tempDefaultFile_);
    }
}

void SimpleSound::setVolume(int volume) noexcept {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    volume_ = volume / 100.0f;
    DWORD vol = static_cast<DWORD>(volume_ * 0xFFFF);
    waveOutSetVolume(NULL, (vol << 16) | vol);
}

void SimpleSound::play() {
    std::wstring filePath = tempSecondFile_.empty() ? tempDefaultFile_ : tempSecondFile_;

    playRequests_.emplace_front(std::async(std::launch::async, [filePath, volume = volume_, tempDefaultFile = tempDefaultFile_]() {
        std::wstring alias = generateAlias();

        if (mciSendStringW((L"open \"" + filePath + L"\" type waveaudio alias " + alias).c_str(), NULL, 0, NULL) != 0) {
            if (mciSendStringW((L"open \"" + tempDefaultFile + L"\" type waveaudio alias " + alias).c_str(), NULL, 0, NULL) != 0) {
                Logger::logWarning("Failed to open WAV file");
                return;
            }
        }

        if (mciSendStringW((L"play " + alias + L" wait").c_str(), NULL, 0, NULL) != 0) {
            Logger::logWarning("Failed to play WAV");
            mciSendStringW((L"close " + alias).c_str(), NULL, 0, NULL);
            return;
        }

        mciSendStringW((L"close " + alias).c_str(), NULL, 0, NULL);
    }));
}

bool SimpleSound::secondBuffer(const std::string& filePath) {
    if (!tempSecondFile_.empty()) {
        tempSecondFile_.clear();
    }

    bool fileExists = fs::exists(filePath);

    if (fileExists) {
        tempSecondFile_ = std::wstring(filePath.begin(), filePath.end());
    }
    else {
        fileExists = fs::exists(filePath + ".WAV");

        if (fileExists) {
            tempSecondFile_ = std::format(L"{}.WAV", std::wstring(filePath.begin(), filePath.end())).c_str();
        }
    }

    return fileExists;
}

bool SimpleSound::loadFromBuffer(std::wstring &tempFile, const unsigned char* data, unsigned int size) {
    if (!data || size == 0) {
        Logger::logWarning("Invalid buffer: null or zero size");
        return false;
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < (std::min)(size, 16u); ++i) {
        oss << std::setw(2) << static_cast<unsigned>(data[i]) << " ";
    }

    tempFile = createTempFile(data, size);
    return !tempFile.empty();
}

std::wstring SimpleSound::createTempFile(const unsigned char* data, unsigned int size) {
    wchar_t tempPath[MAX_PATH];
    wchar_t tempFile[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath) == 0 || GetTempFileNameW(tempPath, L"WAV", 0, tempFile) == 0) {
        Logger::logWarning("Failed to create temporary file path");
        return L"";
    }

    std::ofstream file(tempFile, std::ios::binary);
    if (!file.is_open()) {
        Logger::logWarning("Failed to open temporary file for writing");
        return L"";
    }

    file.write(reinterpret_cast<const char*>(data), size);
    if (!file.good()) {
        Logger::logWarning("Failed to write to temporary file");
        file.close();
        removeTempFile(tempDefaultFile_);
        return L"";
    }

    file.close();

    return std::wstring(tempFile);
}
bool SimpleSound::removeTempFile(std::wstring& tempFile) {
    auto startTime = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(3);

    while (fs::exists(tempFile)) {
        try {
            fs::permissions(tempFile, fs::perms::all, fs::perm_options::remove);

            // Attempt to remove the file
            if (fs::remove(tempFile)) {
                return true;
            }
        }
        catch (...) {}

        // Check if 3 seconds have passed
        if (std::chrono::steady_clock::now() - startTime >= timeout) {
            Logger::logWarning("Failed to remove file within 3 seconds.");
            break;
        }

        // Wait a bit before retrying
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

std::wstring SimpleSound::generateAlias() {
    const std::wstring chars = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);

    std::wstring alias(8, L'\0');
    for (int i = 0; i < 8; ++i) {
        alias[i] = chars[dis(gen)];
    }
    return alias;
}