#pragma once
#include "graphics/crosshair_settings.hpp"

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
    GameConsole(const GameConsole&) = delete;
    GameConsole& operator=(const GameConsole&) = delete;
    void initialize();
    void start();
    bool bind();
    bool ready() const { return bindsReady_; }
    bool takeMatch() { return foundMatch_.exchange(false); }
    bool send(int slot, std::string_view command);

  private:
    std::wstring path_, config_, gsi_, consoleLog_;
    std::filesystem::path videoPath_;
    std::atomic_bool bindsReady_ = false, foundMatch_ = false;
    std::mutex commandMutex_;
    std::jthread worker_;
    crosshair::Settings batch_;
    bool readingConvars_ = false;
    std::chrono::steady_clock::time_point lastConvar_{};

    bool setPaths();
    bool configsValidation();
    void monitor(std::stop_token stop);
    void consume(const std::wstring& line);
};
