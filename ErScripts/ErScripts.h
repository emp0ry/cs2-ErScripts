#pragma once

#include "FileMonitor.h"
#include "GradientManager.h"
#include "Logger.h"
#include "Config.h"
#include "Globals.h"
#include <windows.h>
#include <string>

class ErScripts {
  public:
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

    bool foundMatch = false;

    static bool bindsInit;

    bool setPaths();
    bool configsValidation();

    const std::vector<int> GetPixelColor(int x, int y);
    const bool isColorSimilar(const std::vector<int> &color, const std::vector<int> &targetColor, int range);
};
