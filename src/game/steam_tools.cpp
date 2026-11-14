#include "game/steam_tools.hpp"

#include "core/logger.hpp"
#include "game/cs2_config.hpp"

namespace fs = std::filesystem;

namespace {
    std::wstring readVdf(const fs::path& path) {
        const auto text = cs2config::read(path, 16 * 1024 * 1024);
        std::string_view bytes(text);
        if (bytes.starts_with("\xEF\xBB\xBF"))
            bytes.remove_prefix(3);
        if (bytes.empty())
            return {};
        const auto length = static_cast<int>(bytes.size());
        const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), length, nullptr, 0);
        if (count == 0)
            throw VdfParseError("Steam VDF contains invalid UTF-8");
        std::wstring result(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), length, result.data(), count);
        return result;
    }
} // namespace

VdfParseError::VdfParseError(const std::string& message) : std::runtime_error(message) {}

std::wstring SteamTools::getSteamInstallPath() {
    DWORD bytes = 0;
    constexpr DWORD flags = RRF_RT_REG_SZ | RRF_SUBKEY_WOW6432KEY;
    auto error =
        RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath", flags, nullptr, nullptr, &bytes);
    if (error != ERROR_SUCCESS || bytes < sizeof(wchar_t) || bytes > 32768 * sizeof(wchar_t) ||
        bytes % sizeof(wchar_t) != 0)
        throw std::runtime_error("Invalid Steam InstallPath size");
    std::wstring steamPath(bytes / sizeof(wchar_t), L'\0');
    error = RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath", flags, nullptr,
                         steamPath.data(), &bytes);
    if (error != ERROR_SUCCESS)
        throw std::runtime_error(std::format("Cannot read Steam InstallPath: {}", error));
    const auto end = std::find(std::begin(steamPath), std::end(steamPath), L'\0');
    if (end == std::begin(steamPath) || end == std::end(steamPath))
        throw std::runtime_error("Invalid Steam InstallPath");
    return std::wstring(std::begin(steamPath), end);
}

std::wstring SteamTools::getSteamUserID() {
    DWORD userId = 0;
    DWORD bytes = sizeof(userId);
    const auto error = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam\\ActiveProcess", L"ActiveUser",
                                    RRF_RT_REG_DWORD, nullptr, &userId, &bytes);
    return error == ERROR_SUCCESS && userId != 0 ? std::to_wstring(userId) : std::wstring{};
}

std::string SteamTools::getSteamNickname() {

    std::wstring steamUserID = getSteamUserID();
    if (steamUserID.empty()) {
        throw std::runtime_error("Failed to retrieve Steam UserID.");
    }

    std::wstring installPathW = getSteamInstallPath();

    const auto vdfFilePath = fs::path(installPathW) / L"userdata" / steamUserID / L"config" / L"localconfig.vdf";

    if (!fs::exists(vdfFilePath)) {
        throw std::runtime_error("localconfig.vdf not found.");
    }

    try {

        std::istringstream vdfFile(cs2config::read(vdfFilePath, 16 * 1024 * 1024));
        std::string line;
        while (std::getline(vdfFile, line)) {
            std::istringstream entry(line);
            std::string name, value;
            if ((entry >> std::quoted(name) >> std::quoted(value)) && name == "PersonaName")
                return value;
        }
        throw std::runtime_error("PersonaName not found.");
    } catch (const std::exception& e) {
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
    if (first == std::wstring::npos)
        return L"";
    size_t last = str.find_last_not_of(L" \t\r\n");
    return std::wstring(str.substr(first, last - first + 1));
}

std::map<std::string, std::vector<std::wstring>> SteamTools::parseVdfContent(std::wstring_view content) {
    std::map<std::string, std::vector<std::wstring>> appPaths;
    std::wistringstream iss{std::wstring(content)};
    std::wstring line, currentPath;
    bool inLibraryFolder = false;
    bool inAppsSection = false;

    while (std::getline(iss, line)) {
        line.erase(0, line.find_first_not_of(L" \t"));
        line.erase(line.find_last_not_of(L" \t") + 1);
        if (line.empty())
            continue;

        if (line.size() >= 3 && line.front() == L'"' && line.back() == L'"' &&
            std::all_of(line.begin() + 1, line.end() - 1, [](wchar_t c) { return c >= L'0' && c <= L'9'; })) {
            inLibraryFolder = true;
            currentPath.clear();
            continue;
        }

        if (inLibraryFolder && line == L"{")
            continue;

        if (inLibraryFolder && line.starts_with(L"\"path\"")) {
            size_t start = line.find(L'"', 7);
            size_t end = line.find(L'"', start + 1);
            if (start == std::wstring::npos || end == std::wstring::npos) {
                throw VdfParseError("Malformed library path");
            }
            currentPath = line.substr(start + 1, end - start - 1);
            std::replace(currentPath.begin(), currentPath.end(), L'/', L'\\');
            std::wstring normalizedPath;
            for (size_t i = 0; i < currentPath.length(); ++i) {
                if (i > 0 && currentPath[i] == L'\\' && currentPath[i - 1] == L'\\')
                    continue;
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
                throw VdfParseError("Malformed app entry");
            }

            std::wstring wAppId = line.substr(appIdStart + 1, appIdEnd - appIdStart - 1);
            std::string appId;
            for (wchar_t c : wAppId) {
                if (c < L'0' || c > L'9')
                    throw VdfParseError("Invalid app identifier");
                appId.push_back(static_cast<char>(c));
            }
            if (!appId.empty() && !currentPath.empty()) {
                fs::path basePath = fs::path(currentPath) / L"steamapps" / L"common";
                std::wstring normalizedPath = basePath.wstring();
                std::replace(normalizedPath.begin(), normalizedPath.end(), L'/', L'\\');
                std::wstring singleSlashPath;
                for (size_t i = 0; i < normalizedPath.length(); ++i) {
                    if (i > 0 && normalizedPath[i] == L'\\' && normalizedPath[i - 1] == L'\\')
                        continue;
                    singleSlashPath += normalizedPath[i];
                }
                appPaths[appId].push_back(singleSlashPath);
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

        const auto content = readVdf(vdfPath);
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
                        Logger::logSuccess(
                            std::format(L"Found {} at: {}", std::wstring(appId.begin(), appId.end()), fullPathStr));
                        return fullPathStr;
                    } else {
                        Logger::logInfo(std::format(L"Path {} exists but is not a directory", fullPathStr));
                    }
                }
            }
        }

        Logger::logWarning(std::format(L"AppID {} found in VDF but no valid installation folder located",
                                       std::wstring(appId.begin(), appId.end())));
        return std::nullopt;
    } catch (const std::exception& e) {
        Logger::logWarning(std::format("Error getting app install path: {}", e.what()));
        return std::nullopt;
    }
}

std::filesystem::path SteamTools::getCS2ConfigFolder() {
    const auto user = getSteamConfigPath();
    if (user.empty())
        throw std::runtime_error("Open Steam and sign in before exporting.");
    return fs::path(user) / "730/local/cfg";
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
    std::wistringstream file(readVdf(filePath));

    std::wstring line;
    bool inAppSection = false;
    int braceDepth = 0;
    std::wstring appIdStr(appId.begin(), appId.end());

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
            } else if (line == L"}") {
                braceDepth--;
                if (braceDepth == 0) {
                    inAppSection = false;
                }
            }

            if (braceDepth == 1) {
                std::wistringstream entry(line);
                std::wstring name, value;
                if ((entry >> std::quoted(name) >> std::quoted(value)) && name == L"LaunchOptions")
                    return value;
            }
        }
    }

    return L"";
}

std::optional<SteamTools::Config> SteamTools::getCrosshairSettings(std::string_view appId) {
    try {
        std::wstring steamConfigPath = getSteamConfigPath();
        if (steamConfigPath.empty()) {
            Logger::logWarning(L"Failed to get Steam config path");
            return std::nullopt;
        }

        std::wstring configPath = steamConfigPath + L"\\" + std::wstring(appId.begin(), appId.end()) +
                                  L"\\local\\cfg\\cs2_user_convars_0_slot0.vcfg";

        Logger::logInfo(std::format(L"Looking for crosshair settings at: {}", configPath));

        if (!fs::exists(configPath)) {
            Logger::logWarning(std::format(L"Crosshair config file not found: {}", configPath));
            return std::nullopt;
        }

        Config crosshair = parseCrosshairSettings(configPath);
        if (crosshair.isEmpty()) {
            Logger::logWarning(L"No crosshair settings found in config file");
            return std::nullopt;
        }

        Logger::logSuccess(L"Successfully parsed crosshair settings");
        return crosshair;
    } catch (const std::exception& e) {
        Logger::logWarning(std::format("Error parsing crosshair settings: {}", e.what()));
        return std::nullopt;
    }
}

SteamTools::Config SteamTools::parseCrosshairSettings(const std::wstring& filePath) {
    Config settings;
    std::istringstream file(cs2config::read(filePath));
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string name, value;
        if (stream >> std::quoted(name) >> std::quoted(value))
            crosshair::apply(settings, name, value);
    }
    return settings;
}

void SteamTools::printCrosshairSettings(const std::optional<Config>& settings) {
    if (settings)
        Logger::logInfo(crosshair::commands(*settings));
}
