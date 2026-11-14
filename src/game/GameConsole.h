#pragma once
#include "graphics/CrosshairSettings.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

class GameConsole {
    friend struct GameConsoleTestAccess;

  public:
    GameConsole() = default;
    ~GameConsole();
    GameConsole(const GameConsole &) = delete;
    GameConsole &operator=(const GameConsole &) = delete;
    void initialize();
    void start();
    bool bind();
    bool ready() const { return bindsReady; }
    bool takeMatch() { return foundMatch.exchange(false); }
    bool send(int slot, std::string_view command);

  private:
    std::wstring path, config, gsi, consoleLog;
    std::filesystem::path videoPath;
    std::atomic_bool bindsReady = false, foundMatch = false;
    std::mutex commandMutex;
    std::jthread worker;
    crosshair::Settings batch;
    bool readingConvars = false;
    std::chrono::steady_clock::time_point lastConvar{};

    bool setPaths();
    bool configsValidation();
    void monitor(std::stop_token stop);
    void consume(const std::wstring &line);
};
