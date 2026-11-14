#include "game/game_console.hpp"

#include "core/config_export.hpp"
#include "core/globals.hpp"
#include "core/logger.hpp"
#include "game/game_window.hpp"
#include "game/steam_tools.hpp"
#include "platform/file_monitor.hpp"
#include "platform/input.hpp"

#include <cstdlib>
#include <regex>

namespace fs = std::filesystem;

GameConsole::~GameConsole() {
    worker_.request_stop();
    if (worker_.joinable())
        worker_.join();
}
void GameConsole::initialize() {
    if (setPaths()) {
        Logger::logSuccess(std::format(L"CS2 Path: {}", path_));
    } else {
        Logger::logWarning("Initialization failed: Could not find installation path");
        throw std::runtime_error("CS2 initialization failed");
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
        for (const auto& option : requiredOptions) {
            if (launchOptions.find(option) == std::wstring::npos) {
                isLaunchOptionsGood = false;
            }
        }

        if (!isLaunchOptionsGood) {
            Logger::logWarning("Missing required launch options");
            Logger::logWarning("Add these launch options to make the ErScripts work: -conclearlog -condebug +bind "
                               "scancode104 exec erscripts1");
            throw std::runtime_error("CS2 initialization failed");
        }
    } else {
        Logger::logWarning("No launch options found");
        Logger::logWarning("Add these launch options to make the ErScripts work: -conclearlog -condebug +bind "
                           "scancode104 exec erscripts1");
        throw std::runtime_error("CS2 initialization failed");
    }

    if (game::window()) {
        Logger::logSuccess("CS2 window found");

        if (!validationState) {
            Logger::logWarning("Please restart CS2 as configs validation failed!");
            throw std::runtime_error("CS2 initialization failed");
        }
    }

    try {
        globals::nickname = SteamTools::getSteamNickname();
        Logger::logSuccess(std::format("Steam nickname found: {}", globals::nickname));
    } catch (const std::exception& error) {
        Logger::logWarning(std::format("Steam nickname unavailable: {}", error.what()));
    }

    try {
        videoPath_ = SteamTools::getCS2ConfigFolder() / L"cs2_video.txt";
    } catch (const std::exception&) {
        Logger::logWarning("Video settings unavailable; tracking the CS2 window and console instead");
    }
    Logger::logSuccess("CS2 configuration initialized");
}

bool GameConsole::setPaths() {

    SteamTools::GameConfig cs2Config{.possibleFolders = {
                                         L"Counter-Strike Global Offensive",

                                     }};

    try {
        auto cs2Path = SteamTools::getAppInstallPath("730", cs2Config);
        if (!cs2Path) {
            Logger::logWarning("Failed to locate CS2 installation");
            return false;
        }

        path_ = *cs2Path;

        config_ = path_ + L"/game/csgo/cfg/erscripts";
        gsi_ = path_ + L"/game/csgo/cfg/gamestate_integration_erscripts.cfg";
        consoleLog_ = path_ + L"/game/csgo/console.log";

        if (!fs::exists(path_) || !fs::is_directory(path_)) {
            Logger::logWarning(std::format(L"CS2 path exists but is invalid: {}", path_));
            path_.clear();
            config_.clear();
            gsi_.clear();
            consoleLog_.clear();
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        Logger::logWarning(std::format("[!] Error during path retrieval: {}", e.what()));
        path_.clear();
        config_.clear();
        gsi_.clear();
        consoleLog_.clear();
        return false;
    }
}

bool GameConsole::configsValidation() {
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

    bool returnState = true;

    for (int i = 1; i <= 12; i++) {
        std::wstring filePath = config_ + std::to_wstring(i) + L".cfg";
        std::wifstream cfgFile(filePath);

        if (!cfgFile.good()) {
            std::ofstream file(filePath);
            if (file.is_open()) {
                file.close();
                Logger::logSuccess(std::format(L"Created config: {}", filePath));
            } else {
                Logger::logWarning(std::format(L"Failed to create config: {}", filePath));
                throw std::runtime_error("Cannot create the required CS2 configuration");
            }
        }
    }

    std::wifstream gsiFile(gsi_);

    if (!gsiFile.good()) {
        returnState = false;
        std::ofstream file(gsi_);
        if (file.is_open()) {
            file << gamestateContent;
            file.close();
            if (!file)
                throw std::runtime_error("Cannot write the GSI configuration");
            Logger::logSuccess(std::format(L"Created gsi config: {}", gsi_));
        } else {
            Logger::logWarning(std::format(L"Failed to create gsi config: {}", gsi_));
            throw std::runtime_error("CS2 initialization failed");
        }
    } else {
        returnState = true;
    }

    return returnState;
}

void GameConsole::start() {
    if (!worker_.joinable())
        worker_ = std::jthread([this](std::stop_token stop) {
            try {
                monitor(stop);
            } catch (const std::exception& error) {
                Logger::logWarning(std::format("Console monitoring stopped: {}", error.what()));
            }
        });
}

void GameConsole::monitor(std::stop_token stop) {
    FileMonitor monitor(consoleLog_);
    monitor.start();
    Logger::logInfo("Start CS2 console.log monitoring");
    video::ModeChanges videoChanges;
    fs::file_time_type lastVideoWrite{};
    bool videoReadFailed = false;
    auto nextVideoCheck = std::chrono::steady_clock::time_point{};
    while (!stop.stop_requested() && !globals::finish) {
        const auto now = std::chrono::steady_clock::now();
        if (!videoPath_.empty() && now >= nextVideoCheck) {
            nextVideoCheck = now + std::chrono::milliseconds(200);
            std::error_code error;
            const auto written = fs::last_write_time(videoPath_, error);
            if (!error && written != lastVideoWrite) {
                try {
                    if (const auto size = video::saved(cs2config::read(videoPath_))) {
                        globals::setVideoSize(*size);
                        lastVideoWrite = written;
                    }
                    videoReadFailed = false;
                } catch (const std::exception& failure) {
                    if (!videoReadFailed)
                        Logger::logWarning(std::format("Cannot read video settings: {}", failure.what()));
                    videoReadFailed = true;
                }
            }
        }
        if (auto lines = monitor.getNewLines())
            for (const auto& line : *lines) {
                if (stop.stop_requested() || globals::finish)
                    break;
                if (const auto size = videoChanges.consume(line))
                    globals::setVideoSize(*size);
                consume(line);
            }
        if (readingConvars_ && std::chrono::steady_clock::now() - lastConvar_ > std::chrono::seconds(2))
            readingConvars_ = false;
        std::this_thread::sleep_for(std::chrono::microseconds(15625));
    }
    Logger::logInfo("Stop CS2 console.log monitoring");
}

void GameConsole::consume(const std::wstring& line) {
    static const std::wregex convarPattern(
        LR"((cl_[a-zA-Z0-9_]+|sensitivity|zoom_sensitivity_ratio|m_yaw|m_pitch)\s+"?(-?[0-9]+(?:\.[0-9]+)?|true|false))");
    static const std::wregex regexPing(LR"(latency (\d+) msec)");
    ConfigExport::instance().consume(line);
    if (readingConvars_ && std::chrono::steady_clock::now() - lastConvar_ > std::chrono::seconds(2))
        readingConvars_ = false;
    if (line.find(L"ConVars (non-default):") != std::wstring::npos) {
        batch_ = SteamTools::Config{};
        batch_.modernFields = 15;
        batch_.loaded = true;
        readingConvars_ = true;
        lastConvar_ = std::chrono::steady_clock::now();
    }
    std::wsmatch match;
    if (readingConvars_ && std::regex_search(line, match, convarPattern)) {
        const auto ascii = [](const std::wstring& text) {
            std::string result;
            result.reserve(text.size());
            for (wchar_t c : text)
                result.push_back(static_cast<char>(c));
            return result;
        };
        crosshair::apply(batch_, ascii(match[1].str()), ascii(match[2].str()));
        lastConvar_ = std::chrono::steady_clock::now();
    }
    if (readingConvars_ && line.find(L"[ErScripts] convars_end") != std::wstring::npos) {
        globals::setConfig(batch_);
        readingConvars_ = false;
    }
    if (line.find(L"Server confirmed all players") != std::wstring::npos)
        foundMatch_ = true;
    else if (line.find(L"Misc: rate ") != std::wstring::npos && std::regex_search(line, match, regexPing)) {
        int ping;
        if (std::wistringstream(match[1].str()) >> ping)
            globals::cs2Ping = ping;
    } else if (line.find(L"[*] ErScripts Bind Initialization") != std::wstring::npos ||
               line.find(L"[*] ErScripts Bind Initalization") != std::wstring::npos)
        bindsReady_ = true;
    else if (line.find(L"[Client] source   : slot ") != std::wstring::npos) {
        int slot;
        if ((std::wistringstream(line.substr(line.find(L"slot ") + 5)) >> slot) && slot >= 0)
            globals::localPlayerSlotNumber = slot;
    }
}

bool GameConsole::bind() {
    if (config_.empty() || !game::hasFocus() || !game::cursorHidden() || !globals::getGameState().inGame)
        return false;
    std::lock_guard lock(commandMutex_);
    std::wstring filePath = config_ + L"1.cfg";

    std::fstream configFile(filePath, std::ios::out | std::ios::trunc);
    if (!configFile.is_open()) {
        Logger::logWarning("InitBinds: Failed to open config file!");
        throw std::runtime_error("Cannot initialize the CS2 binds");
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
    command += "echoln [*] ErScripts Bind Initialization";

    configFile.write(command.data(), static_cast<std::streamsize>(command.size()));
    configFile.close();

    if (!configFile)
        throw std::runtime_error("Cannot write the CS2 binds");
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000))
        return false;
    input::KeyPress key(VK_F13);
    if (!key.active())
        return false;
    std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 5) + 1));
    return true;
}
bool GameConsole::send(int num, std::string_view command) {
    // A CFG write and its key pulse form one transaction across all feature workers.
    std::lock_guard lock(commandMutex_);
    if (config_.empty() || num < 1 || num > 12 || !bindsReady_)
        return false;
    if (game::hasFocus() && game::cursorHidden()) {
        std::wstring filePath = config_ + std::to_wstring(num) + L".cfg";

        std::fstream configFile(filePath, std::ios::out | std::ios::trunc);
        if (!configFile.is_open()) {
            Logger::logWarning("CommandsSender: Failed to open config file!");
            return false;
        }

        configFile.write(command.data(), static_cast<std::streamsize>(command.size()));
        configFile.close();
        if (!configFile) {
            Logger::logWarning("CommandsSender: Failed to write config file");
            return false;
        }

        if (!(GetAsyncKeyState(VK_LWIN) & 0x8000) && !(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
            input::KeyPress key(VK_F13 + (num - 1));
            if (!key.active())
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
            return true;
        }
    }
    return false;
}
