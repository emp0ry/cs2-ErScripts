#pragma once

#include "FileMonitor.h"
#include "GradientManager.h"
#include "Logger.h"
#include "Config.h"
#include "Globals.h"
#include <windows.h>
#include <string>
#include <functional>
#include <thread>
#include <vector>

class ErScripts {
  public:
    ErScripts() = default;
    ~ErScripts();
    ErScripts(const ErScripts &) = delete;
    ErScripts &operator=(const ErScripts &) = delete;
    void Initalization();
    void ConsoleLogStream();
    static const bool GetCursorState();
    static const bool GetWindowState();
    static const void GetWindowInfo(int &width, int &height, int &posX, int &posY);
    static bool CommandsSender(const int num, const std::string &command);
    static void Keyboard(int wScan, bool isPressed);

    void InitBinds();
    void AutoAccept();
    void PixelTrigger();
    void Crosshair();
    void BombTimer();
    void RGBCrosshair();
    void KnifeSwitch();
    void AutoPistol();
    void AntiAfk();
    void CS2Binds();
    void KillSay();
    void KillSound();
    void RoundStartAlert();
    void AutoStop();
    void ChatSpammer();

  private:
    static std::atomic<HWND> hwnd;

    std::wstring path;
    static std::wstring config;
    std::wstring gsi;
    std::wstring consoleLog;

    std::atomic_bool foundMatch = false;
    std::vector<std::thread> workers;
    void startWorker(const char *name, std::function<void()> work);

    static std::atomic_bool bindsInit;

    bool setPaths();
    bool configsValidation();

    std::optional<std::array<int, 3>> GetPixelColor(int x, int y);
    bool isColorSimilar(const std::array<int, 3> &color, const std::array<int, 3> &targetColor, int range);
};
