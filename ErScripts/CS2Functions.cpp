#include "CS2Functions.h"
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <thread>

HWND CS2Functions::hwnd = nullptr;
std::wstring CS2Functions::config;
bool CS2Functions::bindsInit = false;

namespace fs = std::filesystem;

void CS2Functions::Initalization() {
    if (setPaths()) {
        Logger::logSuccess(std::format(L"CS2 Path: {}", path));
    }
    else {
        Logger::logWarning("Initialization failed: Could not find installation path");
        std::cout << "[*] Press Enter to exit...";
        std::cin.get();
        std::exit(1);
    }

    bool validationState = configsValidation();

    std::wstring launchOptions = SteamTools::getLaunchOptions("730");
    if (!launchOptions.empty()) {
        Logger::logSuccess(std::format(L"CS2 Launch Options: {}", launchOptions));

        std::vector<std::wstring> requiredOptions = {
            L"+bind scancode104 exec erscripts1",
            L"-conclearlog",
            L"-condebug"
        };

        bool isLaunchOptionsGood = true;
        for (const auto& option : requiredOptions) {
            if (launchOptions.find(option) == std::wstring::npos) {
                isLaunchOptionsGood = false;
            }
        }

        if (!isLaunchOptionsGood) {
            Logger::logWarning("Missing required launch options");
            Logger::logWarning("Add these launch options to make the ErScripts work: -conclearlog -condebug +bind scancode104 exec erscripts1");
            std::cout << "[*] Press Enter to exit...";
            std::cin.get();
            std::exit(1);
        }
    }
    else {
        Logger::logWarning("No launch options found");
        Logger::logWarning("Add these launch options to make the ErScripts work: -conclearlog -condebug +bind scancode104 exec erscripts1");
        std::cout << "[*] Press Enter to exit...";
        std::cin.get();
        std::exit(1);
    }

    if (hwnd = FindWindowA(nullptr, "Counter-Strike 2")) {
        Logger::logSuccess("CS2 window found");

        if (!validationState) {
            Logger::logWarning("Please restart CS2 as configs validation failed!");
            std::cout << "[*] Press Enter to exit...";
            std::cin.get();
            std::exit(1);
        }
    }
    else {
        Logger::logInfo("Please start CS2!");

        while (!(hwnd = FindWindowA(nullptr, "Counter-Strike 2"))) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    globals::nickname = SteamTools::getSteamNickname();
    Logger::logSuccess(std::format("Steam nickname Found: {}", globals::nickname));

    Logger::logSuccess("CS2 initialization completed successfully");
}

void CS2Functions::ConsoleLogStream() {
    std::thread([this]() {
        FileMonitor monitor(consoleLog);
        monitor.start();
        Logger::logInfo("Start CS2 console.log monitoring");

        SteamTools::Crosshair ch_default;
        ch_default.gap = -2.2f;                      // cl_crosshairgap
        ch_default.outlineThickness = 1.0f;          // cl_crosshair_outlinethickness
        ch_default.red = 0;						     // cl_crosshaircolor_r
        ch_default.green = 255;						 // cl_crosshaircolor_g
        ch_default.blue = 0;						 // cl_crosshaircolor_b
        ch_default.alpha = 200;						 // cl_crosshairalpha
        //ch_default.dynamicSplitDist = 3;             // cl_crosshair_dynamic_splitdist
        //ch_default.fixedCrosshairGap = 3.0f;	     // cl_fixedcrosshairgap
        ch_default.color = 5;						 // cl_crosshaircolor
        ch_default.drawOutline = true;				 // cl_crosshair_drawoutline
        //ch_default.dynamicSplitAlphaInnerMod = 0.0f; // cl_crosshair_dynamic_splitalpha_innermod
        //ch_default.dynamicSplitAlphaOuterMod = 1.0f; // cl_crosshair_dynamic_splitalpha_outermod
        //ch_default.dynamicMaxDistSplitRatio = 1.0f;  // cl_crosshair_dynamic_maxdist_splitratio
        ch_default.thickness = 0.6f;				 // cl_crosshairthickness
        ch_default.dot = false;						 // cl_crosshairdot
        //ch_default.gapUseWeaponValue = true;	     // cl_crosshairgap_useweaponvalue
        //ch_default.useAlpha = true;				     // cl_crosshairusealpha
        ch_default.tStyle = false;					 // cl_crosshair_t
        ch_default.style = 2;						 // cl_crosshairstyle
        ch_default.size = 3.9f;					 	 // cl_crosshairsize

        std::wregex regexPing(LR"(latency (\d+) msec)");
        std::wsmatch match;

        while (!globals::finish) {
            if (auto newLines = monitor.getNewLines()) {
                if (newLines && !(*newLines).empty() && (*newLines).size() >= 3) {
                    // Ensure there are at least 3 lines before trying to access them
                    std::wstring combined = (*newLines)[0] + (*newLines)[1] + (*newLines)[2];

                    if (combined.find(L"ConVars (non-default):") != std::wstring::npos) {
                        std::wregex convarPattern(L"^\\S+ \\S+\\s+(\\S+)\\s+(\\S+)\\s+\\S+$");
                        std::wstring foundConVars = L"";

                        for (const auto& line : *newLines) {
                            // Parse and extract crosshair variables from the line
                            std::wsmatch matches;
                            if (std::regex_search(line, matches, convarPattern)) {
                                std::wstring varName = matches[1];
                                std::wstring value = matches[2];
                                foundConVars += varName + L"\n";

                                //Logger::logInfo(std::format(L"Found ConVar: {} {}", varName, value));

                                try {
                                    if (varName == L"cl_crosshairgap") globals::crosshair->gap = std::stof(value);
                                    else if (varName == L"cl_crosshair_outlinethickness") globals::crosshair->outlineThickness = std::stof(value);
                                    else if (varName == L"cl_crosshaircolor_r") globals::crosshair->red = static_cast<uint8_t>(std::stoi(value));
                                    else if (varName == L"cl_crosshaircolor_g") globals::crosshair->green = static_cast<uint8_t>(std::stoi(value));
                                    else if (varName == L"cl_crosshaircolor_b") globals::crosshair->blue = static_cast<uint8_t>(std::stoi(value));
                                    else if (varName == L"cl_crosshairalpha") globals::crosshair->alpha = static_cast<uint8_t>(std::stoi(value));
                                    //else if (varName == L"cl_crosshair_dynamic_splitdist") globals::crosshair->dynamicSplitDist = static_cast<uint8_t>(std::stoi(value));
                                    //else if (varName == L"cl_fixedcrosshairgap") globals::crosshair->fixedCrosshairGap = std::stof(value);
                                    else if (varName == L"cl_crosshaircolor") globals::crosshair->color = static_cast<uint8_t>(std::stoi(value));
                                    else if (varName == L"cl_crosshair_drawoutline") globals::crosshair->drawOutline = (value == L"true" || value == L"1");
                                    //else if (varName == L"cl_crosshair_dynamic_splitalpha_innermod") globals::crosshair->dynamicSplitAlphaInnerMod = std::stof(value);
                                    //else if (varName == L"cl_crosshair_dynamic_splitalpha_outermod") globals::crosshair->dynamicSplitAlphaOuterMod = std::stof(value);
                                    //else if (varName == L"cl_crosshair_dynamic_maxdist_splitratio") globals::crosshair->dynamicMaxDistSplitRatio = std::stof(value);
                                    else if (varName == L"cl_crosshairthickness") globals::crosshair->thickness = std::stof(value);
                                    else if (varName == L"cl_crosshairdot") globals::crosshair->dot = (value == L"true" || value == L"1");
                                    //else if (varName == L"cl_crosshairgap_useweaponvalue") globals::crosshair->gapUseWeaponValue = (value == L"true" || value == L"1");
                                    //else if (varName == L"cl_crosshairusealpha") globals::crosshair->useAlpha = (value == L"true" || value == L"1");
                                    else if (varName == L"cl_crosshair_t") globals::crosshair->tStyle = (value == L"true" || value == L"1");
                                    else if (varName == L"cl_crosshairstyle") globals::crosshair->style = static_cast<uint8_t>(std::stoi(value));
                                    else if (varName == L"cl_crosshairsize") globals::crosshair->size = std::stof(value);
                                }
                                catch (const std::exception& e) {
                                    std::wcout << L"Error parsing " << varName << L": " << e.what() << L"\n";
                                }
                            }
                        }

                        // Apply defaults for missing parameters
                        if (foundConVars.find(L"cl_crosshairgap\n") == std::wstring::npos) globals::crosshair->gap = ch_default.gap;
                        if (foundConVars.find(L"cl_crosshair_outlinethickness\n") == std::wstring::npos) globals::crosshair->outlineThickness = ch_default.outlineThickness;
                        if (foundConVars.find(L"cl_crosshaircolor_r\n") == std::wstring::npos) globals::crosshair->red = ch_default.red;
                        if (foundConVars.find(L"cl_crosshaircolor_g\n") == std::wstring::npos) globals::crosshair->green = ch_default.green;
                        if (foundConVars.find(L"cl_crosshaircolor_b\n") == std::wstring::npos) globals::crosshair->blue = ch_default.blue;
                        if (foundConVars.find(L"cl_crosshairalpha\n") == std::wstring::npos) globals::crosshair->alpha = ch_default.alpha;
                        //if (foundConVars.find(L"cl_crosshair_dynamic_splitdist\n") == std::wstring::npos) globals::crosshair->dynamicSplitDist = ch_default.dynamicSplitDist;
                        //if (foundConVars.find(L"cl_fixedcrosshairgap\n") == std::wstring::npos) globals::crosshair->fixedCrosshairGap = ch_default.fixedCrosshairGap;
                        if (foundConVars.find(L"cl_crosshaircolor\n") == std::wstring::npos) globals::crosshair->color = ch_default.color;
                        if (foundConVars.find(L"cl_crosshair_drawoutline\n") == std::wstring::npos) globals::crosshair->drawOutline = ch_default.drawOutline;
                        //if (foundConVars.find(L"cl_crosshair_dynamic_splitalpha_innermod\n") == std::wstring::npos) globals::crosshair->dynamicSplitAlphaInnerMod = ch_default.dynamicSplitAlphaInnerMod;
                        //if (foundConVars.find(L"cl_crosshair_dynamic_splitalpha_outermod\n") == std::wstring::npos) globals::crosshair->dynamicSplitAlphaOuterMod = ch_default.dynamicSplitAlphaOuterMod;
                        //if (foundConVars.find(L"cl_crosshair_dynamic_maxdist_splitratio\n") == std::wstring::npos) globals::crosshair->dynamicMaxDistSplitRatio = ch_default.dynamicMaxDistSplitRatio;
                        if (foundConVars.find(L"cl_crosshairthickness\n") == std::wstring::npos) globals::crosshair->thickness = ch_default.thickness;
                        if (foundConVars.find(L"cl_crosshairdot\n") == std::wstring::npos) globals::crosshair->dot = ch_default.dot;
                        //if (foundConVars.find(L"cl_crosshairgap_useweaponvalue\n") == std::wstring::npos) globals::crosshair->gapUseWeaponValue = ch_default.gapUseWeaponValue;
                        //if (foundConVars.find(L"cl_crosshairusealpha\n") == std::wstring::npos) globals::crosshair->useAlpha = ch_default.useAlpha;
                        if (foundConVars.find(L"cl_crosshair_t\n") == std::wstring::npos) globals::crosshair->tStyle = ch_default.tStyle;
                        if (foundConVars.find(L"cl_crosshairstyle\n") == std::wstring::npos) globals::crosshair->style = ch_default.style;
                        if (foundConVars.find(L"cl_crosshairsize\n") == std::wstring::npos) globals::crosshair->size = ch_default.size;

                        //SteamTools::printCrosshairSettings(*globals::crosshair);
                        globals::crosshairUpdaterState = false;
                    }
                }

                for (const auto& line : *newLines) {
                    if (line.find(L"Server confirmed all players") != std::wstring::npos) { // Auto Accept
                        foundMatch = true;
                        std::wcout << "Game Found!!!\n";
                    }
                    else if (line.find(L"Misc: rate ") != std::wstring::npos && line.find(L" latency ") != std::wstring::npos) { // Ping
                        if (std::regex_search(line, match, regexPing) && match.size() > 1) {
                            globals::cs2_ping = std::stoi(match[1].str());
                        }
                    }
                    else if (line.find(L"[*] ErScripts Bind Initalization") != std::wstring::npos) { // If ErScripts configs binds set
                        bindsInit = true;
                    }
                    else if (line.find(L"[Client] source   : slot ") != std::wstring::npos) {
                        globals::localPlayerSlotNumber = std::stoi(line.substr(line.find(L"slot ") + wcslen(L"slot ")));
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        monitor.stop();
        Logger::logInfo("Stop CS2 console.log monitoring");
    }).detach();
}


bool CS2Functions::setPaths() {
    // CS2-specific configuration
    SteamTools::GameConfig cs2Config{
        .possibleFolders = {
            L"Counter-Strike Global Offensive",
            L"Counter-Strike 2",
        }
    };

    try {
        auto cs2Path = SteamTools::getAppInstallPath("730", cs2Config);
        if (!cs2Path) {
            Logger::logWarning("Failed to locate CS2 installation");
            return false;
        }

        path = *cs2Path;

        // Set cfg and gsi paths
        config = path + L"/game/csgo/cfg/erscripts";
        gsi = path + L"/game/csgo/cfg/gamestate_integration_erscripts.cfg";
        consoleLog = path + L"/game/csgo/console.log";

        // Verify paths exist
        if (!fs::exists(path) || !fs::is_directory(path)) {
            Logger::logWarning(std::format(L"CS2 path exists but is invalid: {}", path));
            path.clear();
            config.clear();
            gsi.clear();
            consoleLog.clear();
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        Logger::logWarning(std::format("[!] Error during path retrieval: {}", e.what()));
        path.clear();
        config.clear();
        gsi.clear();
        consoleLog.clear();
        return false;
    }
}

bool CS2Functions::configsValidation() {
    std::string gamestateContent = \
R"("GSI Config for ErScripts"
{
    "uri" "http://127.0.0.1:23561"

    "timeout" "5.0"
    "buffer"  "0.0"
    "throttle" "0.1"
    "heartbeat" "10.0"

    "data"
    {
        "map_round_wins" "1"          // history of round wins
        "map" "1"                     // mode, map, phase, team scores
        "player_id" "1"               // steamid
        "player_match_stats" "1"      // scoreboard info
        "player_state" "1"            // armor, flashed, equip_value, health, etc.
        "player_weapons" "1"          // list of player weapons and weapon state
        "provider" "1"               // info about the game providing info
        "round" "1"                   // round phase and the winning team

        // Below this line must be spectating or observing
        "allgrenades" "0"             // grenade effecttime, lifetime, owner, position, type, velocity
        "allplayers_id" "0"           // the steam id of each player
        "allplayers_match_stats" "0"  // the scoreboard info for each player
        "allplayers_position" "0"     // player_position but for each player
        "allplayers_state" "0"        // the player_state for each player
        "allplayers_weapons" "0"      // the player_weapons for each player
        "bomb" "0"                    // location of the bomb, who's carrying it, dropped or not
        "phase_countdowns" "0"        // time remaining in tenths of a second, which phase
        "player_position" "0"         // forward direction, position for currently spectated player
    }
})";

    bool returnState = false;

    std::wifstream cfgFile(config);

    for (int i = 1; i <= 12; i++) {
        std::wstring cfg = config + std::to_wstring(i) + L".cfg";
        std::wifstream cfgFile(cfg);

        if (!cfgFile.good()) {
            std::ofstream file(cfg);
            if (file.is_open()) {
                file.close();
                Logger::logSuccess(std::format(L"Created config: {}", cfg));
            }
            else {
                Logger::logWarning(std::format(L"Failed to create config: {}", cfg));
                std::cout << "[*] Press Enter to exit...";
                std::cin.get();
                std::exit(1);
            }
        }
        else {
            returnState = true;
        }
    }

    std::wifstream gsiFile(gsi);

    if (!gsiFile.good()) {
        std::ofstream file(gsi);
        if (file.is_open()) {
            file << gamestateContent;
            file.close();
            Logger::logSuccess(std::format(L"Created gsi config: {}", gsi));
        }
        else {
            Logger::logWarning(std::format(L"Failed to create gsi config: {}", gsi));
            std::cout << "[*] Press Enter to exit...";
            std::cin.get();
            std::exit(1);
        }
    }
    else {
        returnState = true;
    }
    
    return returnState;
}

void CS2Functions::InitBinds() {
    while (!globals::finish) {
        if (GetAsyncKeyState(VK_END) & 0x8000) globals::finish = true;

        if (GetWindowState() && GetCursorState()) {
            std::wstring cfg = config + +L"1.cfg";

            std::fstream configFile(cfg, std::ios::out | std::ios::trunc);
            if (!configFile.is_open()) {
                Logger::logWarning("InitBinds: Failed to open config file!");
                return;
            }

            std::string command = "bind scancode104 exec erscripts1\n";
            command += "bind scancode105 exec erscripts2\n";
            command += "bind scancode106 exec erscripts3\n";
            command += "bind scancode107 exec erscripts4\n";
            command += "bind scancode108 exec erscripts5\n";
            command += "bind scancode109 exec erscripts6\n";
            command += "bind scancode110 exec erscripts7\n";
            command += "bind scancode111 exec erscripts8\n";
            command += "bind scancode112 exec erscripts9\n";
            command += "bind scancode113 exec erscripts10\n";
            command += "bind scancode114 exec erscripts11\n";
            command += "bind scancode115 exec erscripts12\n";
            command += "echoln [*] ErScripts Bind Initalization";

            configFile.write(command.c_str(), command.length());
            configFile.close();

            if (!(GetAsyncKeyState(VK_LWIN) & 0x8000) && !(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
                Keyboard(VK_F13, true, false);
                std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                Keyboard(VK_F13, false, false);
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto startTime = std::chrono::steady_clock::now();
    const auto timeoutDuration = std::chrono::seconds(5);

    while (!bindsInit) {
        if (GetAsyncKeyState(VK_END) & 0x8000) globals::finish = true;
        if (bindsInit || globals::finish) break;

        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = currentTime - startTime;

        if (elapsedTime >= timeoutDuration) {
            Logger::logWarning("Timeout: Waited 5 seconds for binds initialization or finish signal.");
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (globals::finish || !bindsInit) {
        Logger::logWarning("Initialization failed: Could not initialize erscripts binds");
        std::cout << "[*] Press Enter to exit...";
        std::cin.get();
        std::exit(1);
    }
    else {
        Logger::logSuccess("ErScripts binds initialized");
    }
}

void CS2Functions::CommandsSender(const int num, const std::string& command) {
    if (GetWindowState() && GetCursorState()) {
        if (!bindsInit) {
            Logger::logWarning(std::format("CommandsSender {}: Binds not initialized!!!", num));
            return;
        }
        std::wstring cfg = config + std::to_wstring(num) + L".cfg";

        std::fstream configFile(cfg, std::ios::out | std::ios::trunc);
        if (!configFile.is_open()) {
            Logger::logWarning("CommandsSender: Failed to open config file!");
            return;
        }

        configFile.write(command.c_str(), command.length());
        configFile.close();

        if (!(GetAsyncKeyState(VK_LWIN) & 0x8000) && !(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
            Keyboard(VK_F13 + (num - 1), true, false);
            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
            Keyboard(VK_F13 + (num - 1), false, false);
        }
    }
}

const void CS2Functions::Keyboard(int key, bool isPressed, bool useScanCode) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;

    if (useScanCode) {
        // Use scancode
        input.ki.wScan = static_cast<WORD>(key);
        input.ki.dwFlags = KEYEVENTF_SCANCODE | (isPressed ? 0 : KEYEVENTF_KEYUP);
    }
    else {
        // Use virtual key code
        input.ki.wVk = static_cast<WORD>(key);
        input.ki.dwFlags = (isPressed ? 0 : KEYEVENTF_KEYUP);
    }

    if (SendInput(1, &input, sizeof(INPUT)) != 1) {
        Logger::logWarning(std::format("SendInput failed, Error: {}", GetLastError()));
    }
}

const bool CS2Functions::GetCursorState() {
    CURSORINFO cinfo = { sizeof(CURSORINFO) };
    GetCursorInfo(&cinfo);
    if (cinfo.hCursor)
        return ((WORD)(cinfo.hCursor) > 100);
}

const bool CS2Functions::GetWindowState() {
    return GetForegroundWindow() == hwnd;
}

const void CS2Functions::GetWindowInfo(int& width, int& height, int& posX, int& posY) {
    RECT rect;
    if (GetClientRect(hwnd, &rect)) {
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;

        POINT point = { 0, 0 }; // Top-left of the client area
        if (ClientToScreen(hwnd, &point)) {
            posX = point.x < -20000 ? 0 : point.x;
            posY = point.y < -20000 ? 0 : point.y;
        }
        else {
            posX = posY = 0; // Error handling
        }
    }
    else {
        width = height = 0; // Error handling
    }
}

const std::vector<int> CS2Functions::GetPixelColor(int x, int y) {
    std::vector<int> color(3, -1); // Vector RGB, default value -1

    //Logger::logInfo(std::format("x = {}; y = {}", x, y));

    HDC hdc = GetDC(nullptr);

    if (!hdc)
        return color;

    // Get the pixel color
    COLORREF pixelColor = GetPixel(hdc, x, y);

    color[0] = GetRValue(pixelColor); // red
    color[1] = GetGValue(pixelColor); // green
    color[2] = GetBValue(pixelColor); // blue

    ReleaseDC(nullptr, hdc);

    return color;
}

const bool CS2Functions::isColorSimilar(const std::vector<int>& color, const std::vector<int>& targetColor, int range) {
    if (color.size() != 3 || targetColor.size() != 3) {
        Logger::logWarning("isColorSimilar: Invalid color vector size!");
        return false;
    }

    return (std::abs(color[0] - targetColor[0]) <= range &&
            std::abs(color[1] - targetColor[1]) <= range &&
            std::abs(color[2] - targetColor[2]) <= range);
}