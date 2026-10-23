#include "ErScripts.h"
#include "ConfigExport.h"
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <thread>

std::atomic<HWND> ErScripts::hwnd = nullptr;
std::wstring ErScripts::config;
bool ErScripts::bindsInit = false;

namespace fs = std::filesystem;

void ErScripts::Initalization() {
    if (setPaths()) {
        Logger::logSuccess(std::format(L"CS2 Path: {}", path));
    } else {
        Logger::logWarning("Initialization failed: Could not find installation path");
        std::cout << "[*] Press Enter to exit...";
        std::cin.get();
        std::exit(1);
    }

    if (auto settings = SteamTools::getCrosshairSettings("730"))
        globals::setConfig(*settings);
    bool validationState = configsValidation();

    std::wstring launchOptions = SteamTools::getLaunchOptions("730");
    if (!launchOptions.empty()) {
        Logger::logSuccess(std::format(L"CS2 Launch Options: {}", launchOptions));

        std::vector<std::wstring> requiredOptions = {L"+bind scancode104 exec erscripts1", L"-conclearlog",
                                                     L"-condebug"};

        bool isLaunchOptionsGood = true;
        for (const auto &option : requiredOptions) {
            if (launchOptions.find(option) == std::wstring::npos) {
                isLaunchOptionsGood = false;
            }
        }

        if (!isLaunchOptionsGood) {
            Logger::logWarning("Missing required launch options");
            Logger::logWarning("Add these launch options to make the ErScripts work: -conclearlog -condebug +bind "
                               "scancode104 exec erscripts1");
            std::cout << "[*] Press Enter to exit...";
            std::cin.get();
            std::exit(1);
        }
    } else {
        Logger::logWarning("No launch options found");
        Logger::logWarning("Add these launch options to make the ErScripts work: -conclearlog -condebug +bind "
                           "scancode104 exec erscripts1");
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
    } else {
        Logger::logInfo("Please start CS2!");

        while (!(hwnd = FindWindowA(nullptr, "Counter-Strike 2"))) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    globals::nickname = SteamTools::getSteamNickname();
    Logger::logSuccess(std::format("Steam nickname Found: {}", globals::nickname));

    Logger::logSuccess("CS2 initialization completed successfully");
}

void ErScripts::ConsoleLogStream() {
    std::thread([this]() {
        FileMonitor monitor(consoleLog);
        monitor.start();
        Logger::logInfo("Start CS2 console.log monitoring");

        video::ModeChanges videoChanges;
        fs::path videoPath;
        try {
            videoPath = SteamTools::getCS2ConfigFolder() / L"cs2_video.txt";
        } catch (const std::exception &) {
            Logger::logWarning("Video settings unavailable; tracking the CS2 window and console instead");
        }
        fs::file_time_type lastVideoWrite{};
        auto nextVideoCheck = std::chrono::steady_clock::time_point{};
        SteamTools::Config batch;
        bool readingConvars = false;
        auto lastConvar = std::chrono::steady_clock::now();
        const std::wregex convarPattern(
            LR"((cl_[a-zA-Z0-9_]+|sensitivity|zoom_sensitivity_ratio|m_yaw|m_pitch)\s+"?(-?[0-9]+(?:\.[0-9]+)?|true|false))");
        std::wregex regexPing(LR"(latency (\d+) msec)");
        std::wsmatch match;

        while (!globals::finish) {
            const auto now = std::chrono::steady_clock::now();
            if (!videoPath.empty() && now >= nextVideoCheck) {
                nextVideoCheck = now + std::chrono::milliseconds(200);
                std::error_code error;
                const auto written = fs::last_write_time(videoPath, error);
                if (!error && written != lastVideoWrite) {
                    try {
                        if (const auto size = video::saved(cs2config::read(videoPath))) {
                            globals::setVideoSize(*size);
                            lastVideoWrite = written;
                        }
                    } catch (const std::exception &) {
                    }
                }
            }
            if (auto newLines = monitor.getNewLines()) {
                for (const auto &line : *newLines) {
                    if (const auto size = videoChanges.consume(line))
                        globals::setVideoSize(*size);
                    ConfigExport::instance().consume(line);
                    if (line.find(L"ConVars (non-default):") != std::wstring::npos) {
                        batch = SteamTools::Config{};
                        batch.modernFields = 15;
                        batch.loaded = true;
                        readingConvars = true;
                        lastConvar = std::chrono::steady_clock::now();
                    }
                    std::wsmatch convar;
                    if (readingConvars && std::regex_search(line, convar, convarPattern)) {
                        const auto name = convar[1].str(), value = convar[2].str();
                        const auto ascii = [](const std::wstring &text) {
                            std::string result;
                            for (wchar_t c : text)
                                result.push_back(static_cast<char>(c));
                            return result;
                        };
                        crosshair::apply(batch, ascii(name), ascii(value));
                        lastConvar = std::chrono::steady_clock::now();
                    }
                    if (readingConvars && line.find(L"[ErScripts] convars_end") != std::wstring::npos) {
                        globals::setConfig(batch);
                        readingConvars = false;
                    }
                    if (line.find(L"Server confirmed all players") != std::wstring::npos) {
                        foundMatch = true;
                    } else if (line.find(L"Misc: rate ") != std::wstring::npos &&
                               line.find(L" latency ") != std::wstring::npos) {
                        if (std::regex_search(line, match, regexPing) && match.size() > 1) {
                            globals::cs2_ping = std::stoi(match[1].str());
                        }
                    } else if (line.find(L"[*] ErScripts Bind Initalization") != std::wstring::npos) {
                        bindsInit = true;
                    } else if (line.find(L"[Client] source   : slot ") != std::wstring::npos) {
                        globals::localPlayerSlotNumber = std::stoi(line.substr(line.find(L"slot ") + wcslen(L"slot ")));
                    }
                }
            }
            if (readingConvars && std::chrono::steady_clock::now() - lastConvar > std::chrono::seconds(2)) {
                readingConvars = false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }

        monitor.stop();
        Logger::logInfo("Stop CS2 console.log monitoring");
    }).detach();
}

bool ErScripts::setPaths() {

    SteamTools::GameConfig cs2Config{.possibleFolders = {
                                         L"Counter-Strike Global Offensive",

                                     }};

    try {
        auto cs2Path = SteamTools::getAppInstallPath("730", cs2Config);
        if (!cs2Path) {
            Logger::logWarning("Failed to locate CS2 installation");
            return false;
        }

        path = *cs2Path;

        config = path + L"/game/csgo/cfg/erscripts";
        gsi = path + L"/game/csgo/cfg/gamestate_integration_erscripts.cfg";
        consoleLog = path + L"/game/csgo/console.log";

        if (!fs::exists(path) || !fs::is_directory(path)) {
            Logger::logWarning(std::format(L"CS2 path exists but is invalid: {}", path));
            path.clear();
            config.clear();
            gsi.clear();
            consoleLog.clear();
            return false;
        }

        return true;
    } catch (const std::exception &e) {
        Logger::logWarning(std::format("[!] Error during path retrieval: {}", e.what()));
        path.clear();
        config.clear();
        gsi.clear();
        consoleLog.clear();
        return false;
    }
}

bool ErScripts::configsValidation() {
    std::string gamestateContent =
        R"("GSI Config for ErScripts"
{
    "uri" "http://127.0.0.1:23561"

    "timeout" "5.0"
    "buffer"  "0.0"
    "throttle" "0.1"
    "heartbeat" "10.0"

    "data"
    {
        "map_round_wins" "1"
        "map" "1"
        "player_id" "1"
        "player_match_stats" "1"
        "player_state" "1"
        "player_weapons" "1"
        "provider" "1"
        "round" "1"
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
            } else {
                Logger::logWarning(std::format(L"Failed to create config: {}", cfg));
                std::cout << "[*] Press Enter to exit...";
                std::cin.get();
                std::exit(1);
            }
        } else {
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
        } else {
            Logger::logWarning(std::format(L"Failed to create gsi config: {}", gsi));
            std::cout << "[*] Press Enter to exit...";
            std::cin.get();
            std::exit(1);
        }
    } else {
        returnState = true;
    }

    return returnState;
}

void ErScripts::InitBinds() {
    auto startTime = std::chrono::steady_clock::now();
    const auto timeoutDuration = std::chrono::seconds(10);
    bool timerStart = false;

    int prevExitBind = cfg->erScriptsExitBind;
    std::chrono::steady_clock::time_point changeTime;
    bool isDelayActive = false;

    while (!globals::finish) {
        if (prevExitBind != cfg->erScriptsExitBind) {
            prevExitBind = cfg->erScriptsExitBind;
            changeTime = std::chrono::steady_clock::now();
            isDelayActive = true;
        } else if (isDelayActive) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - changeTime).count();
            if (elapsed >= 2000) {
                isDelayActive = false;
            }
        } else {
            if (GetAsyncKeyState(cfg->erScriptsExitBind) & 0x8000)
                globals::finish = true;
        }

        if (GetWindowState() && GetCursorState() && globals::localPlayerIsActivityPlaying) {
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
                Keyboard(VK_F13, true);
                std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                Keyboard(VK_F13, false);
                startTime = std::chrono::steady_clock::now();
                timerStart = true;
            }
        }

        if (bindsInit || globals::finish)
            break;

        if (timerStart) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = currentTime - startTime;

            if (elapsedTime >= timeoutDuration) {
                Logger::logWarning("Timeout: Waited 10 seconds for binds initialization or finish signal.");
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (globals::finish || !bindsInit) {
        Logger::logWarning("Initialization failed: Could not initialize erscripts binds");
        std::cout << "[*] Press Enter to exit...";
        std::cin.get();
        std::exit(1);
    } else {
        Logger::logSuccess("ErScripts binds initialized");
    }
}

bool ErScripts::CommandsSender(const int num, const std::string &command) {
    if (GetWindowState() && GetCursorState()) {
        if (!bindsInit) {
            Logger::logWarning(std::format("CommandsSender {}: Binds not initialized!!!", num));
            return false;
        }
        std::wstring cfg = config + std::to_wstring(num) + L".cfg";

        std::fstream configFile(cfg, std::ios::out | std::ios::trunc);
        if (!configFile.is_open()) {
            Logger::logWarning("CommandsSender: Failed to open config file!");
            return false;
        }

        configFile.write(command.c_str(), command.length());
        configFile.close();

        if (!(GetAsyncKeyState(VK_LWIN) & 0x8000) && !(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
            Keyboard(VK_F13 + (num - 1), true);
            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
            Keyboard(VK_F13 + (num - 1), false);
            return true;
        }
    }
    return false;
}

void ErScripts::Keyboard(int key, bool isPressed) {
    static HMODULE win32u = LoadLibraryA("win32u.dll");
    static auto NtUserSendInput = reinterpret_cast<UINT(WINAPI *)(UINT, LPINPUT, INT)>(
        win32u ? GetProcAddress(win32u, "NtUserSendInput") : nullptr);

    INPUT input{};

    if (key == VK_LBUTTON || key == VK_RBUTTON || key == VK_MBUTTON || key == VK_XBUTTON1 || key == VK_XBUTTON2) {
        input.type = INPUT_MOUSE;

        if (key == VK_LBUTTON)
            input.mi.dwFlags = isPressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        else if (key == VK_RBUTTON)
            input.mi.dwFlags = isPressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        else if (key == VK_MBUTTON)
            input.mi.dwFlags = isPressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        else {
            input.mi.dwFlags = isPressed ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
            input.mi.mouseData = (key == VK_XBUTTON1) ? XBUTTON1 : XBUTTON2;
        }
    } else {
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = static_cast<WORD>(key);
        input.ki.dwFlags = isPressed ? 0 : KEYEVENTF_KEYUP;
    }

    if (NtUserSendInput) {
        UINT r = NtUserSendInput(1, &input, sizeof(INPUT));
        if (r == 1)
            return;
    }

    SendInput(1, &input, sizeof(INPUT));
}

const bool ErScripts::GetCursorState() {
    CURSORINFO cinfo = {sizeof(CURSORINFO)};
    if (!GetCursorInfo(&cinfo))
        return false;
    return LOWORD(reinterpret_cast<ULONG_PTR>(cinfo.hCursor)) == 0;
}

const bool ErScripts::GetWindowState() {
    const auto target = hwnd.load();
    return target && GetForegroundWindow() == target;
}

const void ErScripts::GetWindowInfo(int &width, int &height, int &posX, int &posY) {
    auto target = hwnd.load();
    static auto nextWindowCheck = std::chrono::steady_clock::time_point{};
    const auto now = std::chrono::steady_clock::now();
    if (!IsWindow(target) || !IsWindowVisible(target) || now >= nextWindowCheck) {
        nextWindowCheck = now + std::chrono::milliseconds(250);
        if (const auto current = FindWindowA(nullptr, "Counter-Strike 2")) {
            target = current;
            hwnd = current;
        }
    }
    RECT rect{};
    POINT point{};
    if (IsIconic(target) || !GetClientRect(target, &rect) || !ClientToScreen(target, &point)) {
        width = height = 0;
        return;
    }
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    posX = point.x;
    posY = point.y;
}

const std::vector<int> ErScripts::GetPixelColor(int x, int y) {
    std::vector<int> color(3, -1);

    HDC hdc = GetDC(nullptr);

    if (!hdc)
        return color;

    COLORREF pixelColor = GetPixel(hdc, x, y);

    color[0] = GetRValue(pixelColor);
    color[1] = GetGValue(pixelColor);
    color[2] = GetBValue(pixelColor);

    ReleaseDC(nullptr, hdc);

    return color;
}

const bool ErScripts::isColorSimilar(const std::vector<int> &color, const std::vector<int> &targetColor, int range) {
    if (color.size() != 3 || targetColor.size() != 3) {
        Logger::logWarning("isColorSimilar: Invalid color vector size!");
        return false;
    }

    return (std::abs(color[0] - targetColor[0]) <= range && std::abs(color[1] - targetColor[1]) <= range &&
            std::abs(color[2] - targetColor[2]) <= range);
}
