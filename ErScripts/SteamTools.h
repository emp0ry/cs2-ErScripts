#pragma once

#include "CrosshairSettings.h"

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <memory>
#include <functional>

class VdfParseError : public std::runtime_error {
  public:
    explicit VdfParseError(const std::string &message);
};

class SteamTools {
  public:
    struct GameConfig {
        std::vector<std::wstring> possibleFolders;
    };

    using Config = crosshair::Settings;

    static std::optional<std::wstring> getAppInstallPath(std::string_view appId, const GameConfig &config);

    static std::wstring getLaunchOptions(std::string_view appId);
    static std::filesystem::path getCS2ConfigFolder();

    static std::string getSteamNickname();

    static std::optional<Config> getCrosshairSettings(std::string_view appId);

    static void printCrosshairSettings(const std::optional<SteamTools::Config> &ch);

  private:
    static std::wstring getSteamInstallPath();

    static std::wstring getSteamUserID();

    static std::wstring getSteamConfigPath();

    static std::map<std::string, std::vector<std::wstring>> parseVdfContent(std::wstring_view content);

    static std::wstring parseLaunchOptions(const std::wstring &filePath, std::string_view appId);

    static std::wstring trim(std::wstring_view str);

    static Config parseCrosshairSettings(const std::wstring &filePath);
};
