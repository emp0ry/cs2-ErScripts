#pragma once

#include "FileMonitor.h"
#include "GradientManager.h"
#include "Logger.h"
#include "Config.h"
#include "Globals.h"
#include <windows.h>
#include <string>

class CS2Functions {
public:
	void Initalization();
	void ConsoleLogStream();
	static const bool GetCursorState();
	static const bool GetWindowState();
	static const void GetWindowInfo(int& width, int& height, int& posX, int& posY);
	static void CommandsSender(const int num, const std::string& command);
	static const void Keyboard(int wScan, bool isPressed, bool useScanCode = true);

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
private:
	static HWND hwnd;

	std::wstring path;				// CS2 installation path
	static std::wstring config;		// Path to cfg file
	std::wstring gsi;				// Path to gamestate_integration file
	std::wstring consoleLog;		// Path to console.log file

	bool foundMatch = false;		// For Auto Accept

	static bool bindsInit;			// Init Binds

	bool setPaths();				// Returns true if path is found and set
	bool configsValidation();		// Configs validation if not exist create

	const std::vector<int> GetPixelColor(int x, int y);
	const bool isColorSimilar(const std::vector<int>& color, const std::vector<int>& targetColor, int range);
};