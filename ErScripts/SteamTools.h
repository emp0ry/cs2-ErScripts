#pragma once

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
    explicit VdfParseError(const std::string& message);
};

class SteamTools {
public:
    // Configuration struct for game-specific folder names
    struct GameConfig {
        std::vector<std::wstring> possibleFolders;
    };

    // CS2 Crosshair struct
    struct Config {
        /* Crosshair */
        float gap = 0.0f;                           // cl_crosshairgap
        float outlineThickness = 0.0f;              // cl_crosshair_outlinethickness
        uint8_t red = 0;                            // cl_crosshaircolor_r
        uint8_t green = 0;                          // cl_crosshaircolor_g
        uint8_t blue = 0;                           // cl_crosshaircolor_b
        uint8_t alpha = 0;                          // cl_crosshairalpha
        //uint8_t dynamicSplitDist = 0;             // cl_crosshair_dynamic_splitdist
        //float fixedCrosshairGap = 0.0f;           // cl_fixedcrosshairgap
        uint8_t color = 0;                          // cl_crosshaircolor
        bool drawOutline = false;                   // cl_crosshair_drawoutline
        //float dynamicSplitAlphaInnerMod = 0.0f;   // cl_crosshair_dynamic_splitalpha_innermod
        //float dynamicSplitAlphaOuterMod = 0.0f;   // cl_crosshair_dynamic_splitalpha_outermod
        //float dynamicMaxDistSplitRatio = 0.0f;    // cl_crosshair_dynamic_maxdist_splitratio
        float thickness = 0.0f;                     // cl_crosshairthickness
        bool dot = false;                           // cl_crosshairdot
        //bool gapUseWeaponValue = false;           // cl_crosshairgap_useweaponvalue
        //bool useAlpha = false;                    // cl_crosshairusealpha
        bool tStyle = false;                        // cl_crosshair_t
        uint8_t style = 0;                          // cl_crosshairstyle
        float size = 0.0f;                          // cl_crosshairsize

        /* Other */
        float sensitivity = 0.0f;                   // sensitivity
        float zoomSensitivity = 0.0f;               // zoom_sensitivity_ratio
        float yaw = 0.0f;                           // m_yaw

        [[nodiscard]] bool isEmpty() const noexcept {
            return size == 0.0f && thickness == 0.0f &&
                red == 0 && green == 0 && blue == 0 && alpha == 0 &&
                !dot && !drawOutline;
        }
    };

    // Get the installation path for a given Steam AppID
    static std::optional<std::wstring> getAppInstallPath(std::string_view appId, const GameConfig& config);

    // Get launch options for a given Steam AppID
    static std::wstring getLaunchOptions(std::string_view appId);

    // Get user nickname
    static std::string getSteamNickname();

    // Get crosshair settings for a given Steam AppID
    static std::optional<Config> getCrosshairSettings(std::string_view appId);

    // Print crosshair settings
    static void printCrosshairSettings(const std::optional<SteamTools::Config>& ch);

private:
    // Get Steam installation path from registry
    static std::wstring getSteamInstallPath();

    // Get Steam active user id from registry
    static std::wstring getSteamUserID();

    // Get Steam config path
    static std::wstring getSteamConfigPath();

    // Parse VDF content into a map of app IDs to their base paths
    static std::map<std::string, std::vector<std::wstring>> parseVdfContent(std::wstring_view content);

    // Parse launch options from a VDF file for a specific AppID
    static std::wstring parseLaunchOptions(const std::wstring& filePath, std::string_view appId);

    // Trim whitespace from a string
    static std::wstring trim(std::wstring_view str);

    // Parse a crosshair configuration from a file
    static Config parseCrosshairSettings(const std::wstring& filePath);
};