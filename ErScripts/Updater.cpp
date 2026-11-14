#include "Updater.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <thread>
#include <random>
#include <array>
#include <charconv>
#include <memory>
#include <optional>

#pragma comment(lib, "wininet.lib")

namespace {
    using InternetHandle = std::unique_ptr<void, decltype(&InternetCloseHandle)>;

    std::optional<std::array<unsigned, 4>> versionParts(std::string_view version) {
        if (version.starts_with('v'))
            version.remove_prefix(1);
        std::array<unsigned, 4> parts{};
        for (auto &part : parts) {
            const auto dot = version.find('.');
            const auto field = version.substr(0, dot);
            const auto parsed = std::from_chars(field.data(), field.data() + field.size(), part);
            if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size())
                return std::nullopt;
            if (dot == version.npos)
                return parts;
            version.remove_prefix(dot + 1);
        }
        return std::nullopt;
    }
} // namespace

bool Updater::newerVersion(std::string_view candidate, std::string_view current) {
    const auto next = versionParts(candidate), previous = versionParts(current);
    return next && previous && *next > *previous;
}

std::string Updater::random_suffix_;

Updater::Updater(const std::string &currentVersion, const std::string &githubAuthor, const std::string &repoName,
                 const std::string &downloadExeName)
    : currentVersion(currentVersion), currentExeName(getCurrentExeName()), downloadExeName(downloadExeName + ".exe"),
      releasesUrl("https://api.github.com/repos/" + githubAuthor + "/" + repoName + "/releases/latest"),
      githubAuthor(githubAuthor), repoName(repoName) {
    Logger::EnableANSIColors();
    if (currentExeName.empty()) {
        Logger::logWarning(std::format("Failed to get current executable name, defaulting to {}", downloadExeName));
        currentExeName = downloadExeName + ".exe";
    }

    if (random_suffix_.empty()) {
        random_suffix_ = generateRandomString(8);
    }
}

std::string Updater::generateRandomString(size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(gen)];
    }
    return result;
}

std::string Updater::getCurrentExeName() {
    char exePath[MAX_PATH];
    DWORD pathLength = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (pathLength == 0 || pathLength >= MAX_PATH) {
        Logger::logWarning(std::format("Failed to get module filename: {}", GetLastError()));
        return "";
    }

    return std::string(exePath, pathLength);
}

std::wstring Updater::stringToWstring(const std::string &str) {
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

std::string Updater::downloadJsonData(const std::string &url) {
    InternetHandle internet(InternetOpenW(L"ProgramUpdater", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0),
                            InternetCloseHandle);
    if (!internet) {
        Logger::logWarning(std::format("Failed to initialize WinINet: {}", GetLastError()));
        return {};
    }
    const auto wideUrl = stringToWstring(url);
    InternetHandle request(InternetOpenUrlW(internet.get(), wideUrl.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0),
                           InternetCloseHandle);
    DWORD status = 0, length = sizeof(status);
    if (!request ||
        !HttpQueryInfoW(request.get(), HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &length, nullptr) ||
        status != HTTP_STATUS_OK) {
        Logger::logWarning("Cannot read the latest release information");
        return {};
    }
    std::string data;
    char buffer[4096];
    DWORD bytes = 0;
    do {
        if (!InternetReadFile(request.get(), buffer, sizeof(buffer), &bytes)) {
            Logger::logWarning("Release information download failed");
            return {};
        }
        if (data.size() + bytes > 4 * 1024 * 1024) {
            Logger::logWarning("Release information exceeded 4 MiB");
            return {};
        }
        data.append(buffer, bytes);
    } while (bytes);
    return data;
}

bool Updater::downloadFile(const std::string &url, const std::string &outputPath) {
    InternetHandle internet(InternetOpenW(L"ProgramUpdater", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0),
                            InternetCloseHandle);
    if (!internet) {
        Logger::logWarning(std::format("Failed to initialize WinINet: {}", GetLastError()));
        return false;
    }
    const auto wideUrl = stringToWstring(url);
    InternetHandle request(InternetOpenUrlW(internet.get(), wideUrl.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0),
                           InternetCloseHandle);
    DWORD status = 0, length = sizeof(status);
    if (!request ||
        !HttpQueryInfoW(request.get(), HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &length, nullptr) ||
        status != HTTP_STATUS_OK) {
        Logger::logWarning("Cannot download the update");
        return false;
    }
    DWORD expected = 0;
    length = sizeof(expected);
    HttpQueryInfoW(request.get(), HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &expected, &length, nullptr);
    std::ofstream file(outputPath, std::ios::binary);
    char buffer[4096];
    DWORD bytes = 0;
    size_t total = 0;
    bool downloaded = false;
    while (file) {
        if (!InternetReadFile(request.get(), buffer, sizeof(buffer), &bytes))
            break;
        if (!bytes) {
            downloaded = true;
            break;
        }
        file.write(buffer, bytes);
        total += bytes;
    }
    file.close();
    if (!downloaded || !file || (expected && total != expected)) {
        Logger::logWarning("Update download was incomplete or could not be saved");
        std::error_code error;
        std::filesystem::remove(outputPath, error);
        return false;
    }
    Logger::logSuccess(std::format("Downloaded {} bytes to {}", total, outputPath));
    return true;
}

bool Updater::isValidExecutable(const std::string &filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        Logger::logWarning(std::format("Cannot open file for validation: {}", filePath));
        return false;
    }

    char header[2]{};
    file.read(header, 2);
    const bool isValid = file.gcount() == 2 && header[0] == 'M' && header[1] == 'Z';
    file.close();
    if (!isValid) {
        Logger::logWarning(std::format("File is not a valid PE executable: {}", filePath));
    }
    return isValid;
}

bool Updater::replaceExecutable(const std::string &newFilePath) {
    if (!isValidExecutable(newFilePath)) {
        Logger::logWarning("Downloaded file is not a valid executable");
        return false;
    }

    std::string backupPath = currentExeName + "_old.exe";
    std::string currentPath = currentExeName;

    char temp_dir[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_dir) == 0) {
        Logger::logWarning(std::format("Failed to get TEMP path: {}", GetLastError()));
        return false;
    }
    std::string batPath = std::string(temp_dir) + "update_" + random_suffix_ + ".bat";

    for (int retry = 0; retry < 3; ++retry) {
        std::ofstream batFile(batPath);
        if (batFile) {
            batFile << "@echo off\n";
            batFile << "timeout /t 1 >nul\n";
            batFile << "move /Y \"" << currentPath << "\" \"" << backupPath << "\"\n";
            batFile << "move /Y \"" << newFilePath << "\" \"" << currentPath << "\"\n";
            batFile << "if exist \"" << currentPath << "\" (\n";
            batFile << "    start \"\" \"" << currentPath << "\"\n";
            batFile << "    del \"" << backupPath << "\"\n";
            batFile << "    del \"%~f0\"\n";
            batFile << ")\n";
            batFile.close();
            break;
        }
        Logger::logWarning(std::format("Retry {}: Failed to create batch file: {}", retry + 1, GetLastError()));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!std::filesystem::exists(batPath)) {
        Logger::logWarning("Failed to create batch file after retries");
        return false;
    }

    SHELLEXECUTEINFOA sei = {sizeof(sei)};
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    sei.lpFile = batPath.c_str();
    sei.lpVerb = "open";
    sei.nShow = SW_HIDE;
    if (!ShellExecuteExA(&sei)) {
        Logger::logWarning(std::format("Failed to execute batch file: {}", GetLastError()));
        std::filesystem::remove(batPath);
        return false;
    }

    if (sei.hProcess)
        CloseHandle(sei.hProcess);
    return true;
}

void Updater::cleanupTempFiles() {
    char temp_dir[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_dir) == 0) {
        Logger::logWarning(std::format("Failed to get TEMP path for cleanup: {}", GetLastError()));
        return;
    }

    std::string temp_exe = std::string(temp_dir) + "ErScripts_new_" + random_suffix_ + ".exe";
    std::string temp_bat = std::string(temp_dir) + "update_" + random_suffix_ + ".bat";

    if (std::filesystem::exists(temp_exe)) {
        std::filesystem::remove(temp_exe);
        Logger::logInfo(std::format("Cleaned up temp file: {}", temp_exe));
    }
    if (std::filesystem::exists(temp_bat)) {
        std::filesystem::remove(temp_bat);
        Logger::logInfo(std::format("Cleaned up batch file: {}", temp_bat));
    }
}

bool Updater::checkAndUpdate() {
    std::string jsonData = downloadJsonData(releasesUrl);
    if (jsonData.empty()) {
        Logger::logWarning("Could not check for updates.");
        return false;
    }

    try {
        auto json = nlohmann::json::parse(jsonData);
        std::string latestVersion = json.at("tag_name").get<std::string>();
        if (latestVersion.starts_with('v'))
            latestVersion.erase(0, 1);

        if (newerVersion(latestVersion, currentVersion)) {
            Logger::logInfo(std::format("New version available: {} (current: {})", latestVersion, currentVersion));

            std::string downloadUrl = "https://github.com/" + githubAuthor + "/" + repoName + "/releases/download/v" +
                                      latestVersion + "/" + downloadExeName;

            char temp_dir[MAX_PATH];
            if (GetTempPathA(MAX_PATH, temp_dir) == 0) {
                Logger::logWarning(std::format("Failed to get TEMP path: {}", GetLastError()));
                return false;
            }
            std::string tempPath = std::string(temp_dir) + "ErScripts_new_" + random_suffix_ + ".exe";

            if (downloadFile(downloadUrl, tempPath)) {
                if (replaceExecutable(tempPath)) {
                    Logger::logSuccess("Update initiated. Restarting...");
                    return true;
                }
            }
            std::filesystem::remove(tempPath);
            return false;
        } else {
            return false;
        }
    } catch (const nlohmann::json::exception &e) {
        Logger::logWarning(std::format("JSON parsing error: {}", e.what()));
        return false;
    }
}
