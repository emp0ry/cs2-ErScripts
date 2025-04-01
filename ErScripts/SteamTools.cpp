#include "SteamTools.h"
#include "Logger.h"

namespace fs = std::filesystem;

// VdfParseError implementation
VdfParseError::VdfParseError(const std::string& message)
    : std::runtime_error(message) {}

std::wstring SteamTools::getSteamInstallPath() {
    HKEY hKey{};
    wchar_t steamPath[256]{};
    DWORD pathSize = sizeof(steamPath);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        throw std::runtime_error("Failed to open Steam registry key");
    }

    // Use a custom deleter function
    auto keyDeleter = [](HKEY* key) {
        if (key && *key) {
            RegCloseKey(*key);
        }
    };
    std::unique_ptr<HKEY, decltype(keyDeleter)> keyGuard(&hKey, keyDeleter);

    if (RegQueryValueEx(hKey, L"InstallPath", nullptr, nullptr, reinterpret_cast<LPBYTE>(steamPath), &pathSize) != ERROR_SUCCESS) {
        throw std::runtime_error("Failed to read Steam InstallPath from registry");
    }

    Logger::logSuccess(std::format(L"Steam install path from registry: {}", steamPath));
    return std::wstring(steamPath);
}

std::wstring SteamTools::getSteamUserID() {
    HKEY hKey{};
    DWORD userID;
    DWORD size = sizeof(userID);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Valve\\Steam\\ActiveProcess", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, L"ActiveUser", nullptr, nullptr, (LPBYTE)&userID, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::to_wstring(userID);
        }
        RegCloseKey(hKey);
    }

    return L"";
}

std::string SteamTools::getSteamNickname() {
    // Get the Steam user ID as a wide string
    std::wstring steamUserID = getSteamUserID();
    if (steamUserID.empty()) {
        throw std::runtime_error("Failed to retrieve Steam UserID.");
    }

    // Convert the Steam install path (wstring) and user ID (wstring) to std::string
    std::wstring installPathW = getSteamInstallPath();
    std::string installPath(installPathW.begin(), installPathW.end());
    std::string steamUserIDStr(steamUserID.begin(), steamUserID.end());

    // Construct the path to the localconfig.vdf file
    std::string vdfFilePath = installPath + "\\userdata\\" + steamUserIDStr + "\\config\\localconfig.vdf";

    if (!fs::exists(vdfFilePath)) {
        throw std::runtime_error("localconfig.vdf not found.");
    }

    try {
        // Open the VDF file and read it line by line
        std::ifstream vdfFile(vdfFilePath);
        std::string line;
        std::string personaName;
        bool foundPersonaName = false;

        while (std::getline(vdfFile, line)) {
            // Look for the line with "PersonaName"
            std::size_t keyPos = line.find("\"PersonaName\"");
            if (keyPos != std::string::npos) {
                // Find the first quote after the key
                std::size_t valueStartQuote = line.find("\"", keyPos + std::string("\"PersonaName\"").length());
                if (valueStartQuote != std::string::npos) {
                    // Find the ending quote after the starting quote
                    std::size_t valueEndQuote = line.find("\"", valueStartQuote + 1);
                    if (valueEndQuote != std::string::npos) {
                        personaName = line.substr(valueStartQuote + 1, valueEndQuote - valueStartQuote - 1);
                        foundPersonaName = true;
                        break;
                    }
                }
            }
        }

        if (!foundPersonaName) {
            throw std::runtime_error("PersonaName not found.");
        }

        return personaName;
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error reading localconfig.vdf: " + std::string(e.what()));
    }
}

std::wstring SteamTools::getSteamConfigPath() {
    std::wstring steamPath = getSteamInstallPath();
    if (steamPath.empty()) {
        return L"";
    }

    std::wstring steamUserID = getSteamUserID();
    if (steamUserID.empty()) {
        return L"";
    }

    return steamPath + L"\\userdata\\" + steamUserID;
}

std::wstring SteamTools::trim(std::wstring_view str) {
    size_t first = str.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    size_t last = str.find_last_not_of(L" \t\r\n");
    return std::wstring(str.substr(first, last - first + 1));
}

std::map<std::string, std::vector<std::wstring>> SteamTools::parseVdfContent(std::wstring_view content) {
    std::map<std::string, std::vector<std::wstring>> appPaths;
    std::wistringstream iss(content.data());
    std::wstring line, currentPath;
    bool inLibraryFolder = false;
    bool inAppsSection = false;

    while (std::getline(iss, line)) {
        line.erase(0, line.find_first_not_of(L" \t")); // Trim left
        line.erase(line.find_last_not_of(L" \t") + 1); // Trim right
        if (line.empty()) continue;

        if (line.size() >= 3 && line[0] == L'"' && iswdigit(line[1]) && line[2] == L'"') {
            inLibraryFolder = true;
            currentPath.clear();
            continue;
        }

        if (inLibraryFolder && line == L"{") continue;

        if (inLibraryFolder && line.starts_with(L"\"path\"")) {
            size_t start = line.find(L'"', 7);
            size_t end = line.find(L'"', start + 1);
            if (start == std::wstring::npos || end == std::wstring::npos) {
                throw VdfParseError("Malformed path line: " + std::string(line.begin(), line.end()));
            }
            currentPath = line.substr(start + 1, end - start - 1);
            std::replace(currentPath.begin(), currentPath.end(), L'/', L'\\');
            std::wstring normalizedPath;
            for (size_t i = 0; i < currentPath.length(); ++i) {
                if (i > 0 && currentPath[i] == L'\\' && currentPath[i - 1] == L'\\') continue;
                normalizedPath += currentPath[i];
            }
            currentPath = normalizedPath;
            Logger::logSuccess(std::format(L"Found library path: {}", currentPath));
            continue;
        }

        if (inLibraryFolder && line == L"\"apps\"") {
            inAppsSection = true;
            continue;
        }

        if (inAppsSection && line == L"}") {
            inAppsSection = false;
            continue;
        }

        if (inLibraryFolder && !inAppsSection && line == L"}") {
            inLibraryFolder = false;
            continue;
        }

        if (inAppsSection && line.starts_with(L"\"")) {
            size_t appIdStart = line.find(L'"');
            size_t appIdEnd = line.find(L'"', appIdStart + 1);
            size_t sizeStart = line.find(L'"', appIdEnd + 1);
            size_t sizeEnd = line.find(L'"', sizeStart + 1);
            if (appIdEnd == std::wstring::npos || sizeEnd == std::wstring::npos) {
                throw VdfParseError("Malformed app entry: " + std::string(line.begin(), line.end()));
            }

            std::wstring wAppId = line.substr(appIdStart + 1, appIdEnd - appIdStart - 1);
            std::string appId(wAppId.begin(), wAppId.end());
            if (!appId.empty() && !currentPath.empty()) {
                fs::path basePath = fs::path(currentPath) / L"steamapps" / L"common";
                std::wstring normalizedPath = basePath.wstring();
                std::replace(normalizedPath.begin(), normalizedPath.end(), L'/', L'\\');
                std::wstring singleSlashPath;
                for (size_t i = 0; i < normalizedPath.length(); ++i) {
                    if (i > 0 && normalizedPath[i] == L'\\' && normalizedPath[i - 1] == L'\\') continue;
                    singleSlashPath += normalizedPath[i];
                }
                appPaths[appId].push_back(singleSlashPath);
                // Logger::logInfo(std::format(L"App {} potentially at: {}", wAppId, singleSlashPath));
            }
        }
    }

    return appPaths;
}

std::optional<std::wstring> SteamTools::getAppInstallPath(std::string_view appId, const GameConfig& config) {
    try {
        std::wstring steamPath = getSteamInstallPath();
        if (steamPath.empty()) {
            Logger::logWarning(L"Could not determine Steam installation path");
            return std::nullopt;
        }
        Logger::logSuccess(std::format(L"Steam install path from registry: {}", steamPath));

        fs::path vdfPath = fs::path(steamPath) / L"steamapps" / L"libraryfolders.vdf";
        std::wstring vdfPathStr = vdfPath.wstring();
        std::replace(vdfPathStr.begin(), vdfPathStr.end(), L'/', L'\\');
        Logger::logInfo(std::format(L"Looking for VDF at: {}", vdfPathStr));

        std::wifstream file(vdfPath);
        if (!file.is_open()) {
            Logger::logWarning(L"Failed to open VDF file: " + vdfPathStr);
            return std::nullopt;
        }

        std::wstringstream wss;
        wss << file.rdbuf();
        std::wstring content = wss.str();
        file.close();
        Logger::logSuccess(L"Successfully read VDF file");

        auto appPaths = parseVdfContent(content);
        auto it = appPaths.find(std::string(appId));
        if (it == appPaths.end()) {
			Logger::logWarning(std::format(L"AppID {} not found in VDF", std::wstring(appId.begin(), appId.end())));
            return std::nullopt;
        }

        const auto& potentialPaths = it->second;
        for (const auto& basePath : potentialPaths) {
            Logger::logInfo(std::format(L"Exploring library path: {}", basePath));
            for (const auto& folder : config.possibleFolders) {
                fs::path fullPath = fs::path(basePath) / folder;
                std::wstring fullPathStr = fullPath.wstring();
                std::replace(fullPathStr.begin(), fullPathStr.end(), L'/', L'\\');
                Logger::logInfo(std::format(L"Checking: {}", fullPathStr));
                if (fs::exists(fullPath)) {
                    if (fs::is_directory(fullPath)) {
                        Logger::logSuccess(std::format(L"Found {} at: {}",
                            std::wstring(appId.begin(), appId.end()), fullPathStr));
                        return fullPathStr;
                    }
                    else {
                        Logger::logInfo(std::format(L"Path {} exists but is not a directory", fullPathStr));
                    }
                }
            }
        }

        Logger::logWarning(std::format(L"AppID {} found in VDF but no valid installation folder located",
            std::wstring(appId.begin(), appId.end())));
        return std::nullopt;
    }
    catch (const std::exception& e) {
		Logger::logWarning(std::format("Error getting app install path: {}", e.what()));
        return std::nullopt;
    }
}

std::wstring SteamTools::getLaunchOptions(std::string_view appId) {
    std::wstring steamConfigPath = getSteamConfigPath();
    if (steamConfigPath.empty()) {
		Logger::logWarning(L"Failed to get Steam config path.");
        return L"";
    }

    std::wstring localConfigPath = steamConfigPath + L"\\config\\localconfig.vdf";
    return parseLaunchOptions(localConfigPath, appId);
}

std::wstring SteamTools::parseLaunchOptions(const std::wstring& filePath, std::string_view appId) {
    std::wifstream file(filePath);
    if (!file.is_open()) {
        Logger::logWarning(L"Failed to open: " + filePath);
        return L"";
    }

    std::wstring line;
    bool inAppSection = false;
    int braceDepth = 0;
    std::wstring appIdStr(appId.begin(), appId.end());
    std::wregex launchOptionsRegex(L"\"LaunchOptions\"\\s+\"([^\"]+)\"");

    while (std::getline(file, line)) {
        line = trim(line);

        if (!inAppSection && line == L"\"" + appIdStr + L"\"") {
            inAppSection = true;
            braceDepth = 0;
            continue;
        }

        if (inAppSection) {
            if (line == L"{") {
                braceDepth++;
            }
            else if (line == L"}") {
                braceDepth--;
                if (braceDepth == 0) {
                    inAppSection = false;
                }
            }

            // Only match LaunchOptions when at depth 1 (main app section)
            if (braceDepth == 1) {
                std::wsmatch match;
                if (std::regex_search(line, match, launchOptionsRegex)) {
                    file.close();
                    return match[1].str();
                }
            }
        }
    }

    file.close();
    return L"";
}

std::optional<SteamTools::Crosshair> SteamTools::getCrosshairSettings(std::string_view appId) {
    try {
        std::wstring steamConfigPath = getSteamConfigPath();
        if (steamConfigPath.empty()) {
            Logger::logWarning(L"Failed to get Steam config path");
            return std::nullopt;
        }

        // Construct the path to CS2 user convars file
        std::wstring configPath = steamConfigPath + L"\\" +
            std::wstring(appId.begin(), appId.end()) +
            L"\\local\\cfg\\cs2_user_convars_0_slot0.vcfg";

        Logger::logInfo(std::format(L"Looking for crosshair settings at: {}", configPath));

        if (!fs::exists(configPath)) {
            Logger::logWarning(std::format(L"Crosshair config file not found: {}", configPath));
            return std::nullopt;
        }

        Crosshair crosshair = parseCrosshairSettings(configPath);
        if (crosshair.isEmpty()) {
            Logger::logWarning(L"No crosshair settings found in config file");
            return std::nullopt;
        }

        Logger::logSuccess(L"Successfully parsed crosshair settings");
        return crosshair;
    }
    catch (const std::exception& e) {
        Logger::logWarning(std::format("Error parsing crosshair settings: {}", e.what()));
        return std::nullopt;
    }
}

SteamTools::Crosshair SteamTools::parseCrosshairSettings(const std::wstring& filePath) {
    Crosshair crosshair;
    
    std::wifstream file(filePath);
    if (!file.is_open()) {
        Logger::logWarning(std::format(L"Failed to open crosshair config: {}", filePath));
        return crosshair;
    }

    std::wstringstream wss;
    wss << file.rdbuf();
    std::wstring content = wss.str();
    
    std::map<std::wstring, std::function<void(Crosshair&, const std::wstring&)>> convarMap {
        {L"cl_crosshairgap", [&](Crosshair& ch, const std::wstring& val) { ch.gap = std::stof(val); }},
        {L"cl_crosshair_outlinethickness", [&](Crosshair& ch, const std::wstring& val) { ch.outlineThickness = std::stof(val); }},
        {L"cl_crosshaircolor_r", [&](Crosshair& ch, const std::wstring& val) { ch.red = static_cast<uint8_t>(std::stoi(val)); }},
        {L"cl_crosshaircolor_g", [&](Crosshair& ch, const std::wstring& val) { ch.green = static_cast<uint8_t>(std::stoi(val)); }},
        {L"cl_crosshaircolor_b", [&](Crosshair& ch, const std::wstring& val) { ch.blue = static_cast<uint8_t>(std::stoi(val)); }},
        {L"cl_crosshairalpha", [&](Crosshair& ch, const std::wstring& val) { ch.alpha = static_cast<uint8_t>(std::stoi(val)); }},
        //{L"cl_crosshair_dynamic_splitdist", [&](Crosshair& ch, const std::wstring& val) { ch.dynamicSplitDist = static_cast<uint8_t>(std::stoi(val)); }},
        //{L"cl_fixedcrosshairgap", [&](Crosshair& ch, const std::wstring& val) { ch.fixedCrosshairGap = std::stof(val); }},
        {L"cl_crosshaircolor", [&](Crosshair& ch, const std::wstring& val) { ch.color = static_cast<uint8_t>(std::stoi(val)); }},
        {L"cl_crosshair_drawoutline", [&](Crosshair& ch, const std::wstring& val) { ch.drawOutline = val == L"true" || val == L"1"; }},
        //{L"cl_crosshair_dynamic_splitalpha_innermod", [&](Crosshair& ch, const std::wstring& val) { ch.dynamicSplitAlphaInnerMod = std::stof(val); }},
        //{L"cl_crosshair_dynamic_splitalpha_outermod", [&](Crosshair& ch, const std::wstring& val) { ch.dynamicSplitAlphaOuterMod = std::stof(val); }},
        //{L"cl_crosshair_dynamic_maxdist_splitratio", [&](Crosshair& ch, const std::wstring& val) { ch.dynamicMaxDistSplitRatio = std::stof(val); }},
        {L"cl_crosshairthickness", [&](Crosshair& ch, const std::wstring& val) { ch.thickness = std::stof(val); }},
        {L"cl_crosshairdot", [&](Crosshair& ch, const std::wstring& val) { ch.dot = val == L"true" || val == L"1"; }},
        //{L"cl_crosshairgap_useweaponvalue", [&](Crosshair& ch, const std::wstring& val) { ch.gapUseWeaponValue = val == L"true" || val == L"1"; }},
        //{L"cl_crosshairusealpha", [&](Crosshair& ch, const std::wstring& val) { ch.useAlpha = val == L"true" || val == L"1"; }},
        {L"cl_crosshair_t", [&](Crosshair& ch, const std::wstring& val) { ch.tStyle = val == L"true" || val == L"1"; }},
        {L"cl_crosshairstyle", [&](Crosshair& ch, const std::wstring& val) { ch.style = static_cast<uint8_t>(std::stoi(val)); }},
        {L"cl_crosshairsize", [&](Crosshair& ch, const std::wstring& val) { ch.size = std::stof(val); }}
    };

    std::wistringstream iss(content);
    std::wstring line;
    bool inConvars = false;
    int braceDepth = 0;

    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (line == L"\"convars\"") {
            inConvars = true;
            continue;
        }

        if (inConvars) {
            if (line == L"{") {
                braceDepth++;
                continue;
            }
            if (line == L"}") {
                braceDepth--;
                if (braceDepth == 0) {
                    inConvars = false;
                    break;
                }
                continue;
            }

            if (line.starts_with(L"\"cl_crosshair") || line.starts_with(L"\"cl_fixedcrosshair")) {
                std::wistringstream lineStream(line);
                std::wstring key, value;
                if (lineStream >> std::quoted(key) >> std::quoted(value)) {
                    auto it = convarMap.find(key);
                    if (it != convarMap.end()) {
                        try {
                            it->second(crosshair, value);
                            Logger::logInfo(std::format(L"Parsed: {} \"{}\"", key, value)); // Log during parsing
                        }
                        catch (const std::exception& e) {
                            std::string narrowWhat = e.what();
                            std::wstring wideWhat(narrowWhat.begin(), narrowWhat.end());
                            Logger::logWarning(std::format(L"Error parsing {} with value {}: {}", key, value, wideWhat));
                        }
                    }
                }
            }
        }
    }

    return crosshair;
}

void SteamTools::printCrosshairSettings(const std::optional<SteamTools::Crosshair>& ch) {
	Logger::logSuccess("Crosshair settings:");
    Logger::logInfo(std::format("cl_crosshairgap \"{:.1f}\"", ch->gap));
    Logger::logInfo(std::format("cl_crosshair_outlinethickness \"{:.1f}\"", ch->outlineThickness));
    Logger::logInfo(std::format("cl_crosshaircolor_r \"{}\"", ch->red));
    Logger::logInfo(std::format("cl_crosshaircolor_g \"{}\"", ch->green));
    Logger::logInfo(std::format("cl_crosshaircolor_b \"{}\"", ch->blue));
    Logger::logInfo(std::format("cl_crosshairalpha \"{}\"", ch->alpha));
    //Logger::logInfo(std::format("cl_crosshair_dynamic_splitdist \"{}\"", ch->dynamicSplitDist));
    //Logger::logInfo(std::format("cl_fixedcrosshairgap \"{:.1f}\"", ch->fixedCrosshairGap));
    Logger::logInfo(std::format("cl_crosshaircolor \"{}\"", ch->color));
    Logger::logInfo(std::format("cl_crosshair_drawoutline \"{}\"", ch->drawOutline ? 1 : 0));
    //Logger::logInfo(std::format("cl_crosshair_dynamic_splitalpha_innermod \"{:.1f}\"", ch->dynamicSplitAlphaInnerMod));
    //Logger::logInfo(std::format("cl_crosshair_dynamic_splitalpha_outermod \"{:.1f}\"", ch->dynamicSplitAlphaOuterMod));
    //Logger::logInfo(std::format("cl_crosshair_dynamic_maxdist_splitratio \"{:.1f}\"", ch->dynamicMaxDistSplitRatio));
    Logger::logInfo(std::format("cl_crosshairthickness \"{:.1f}\"", ch->thickness));
    Logger::logInfo(std::format("cl_crosshairdot \"{}\"", ch->dot ? 1 : 0));
    //Logger::logInfo(std::format("cl_crosshairgap_useweaponvalue \"{}\"", ch->gapUseWeaponValue ? 1 : 0));
    //Logger::logInfo(std::format("cl_crosshairusealpha \"{}\"", ch->useAlpha ? 1 : 0));
    Logger::logInfo(std::format("cl_crosshair_t \"{}\"", ch->tStyle ? 1 : 0));
    Logger::logInfo(std::format("cl_crosshairstyle \"{}\"", ch->style));
    Logger::logInfo(std::format("cl_crosshairsize \"{:.1f}\"", ch->size));
}