#include "Rebuild.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>
#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

std::string Rebuild::random_suffix_; // Initialize static member

Rebuild::Rebuild() {
    // Generate suffix once per run
    if (random_suffix_.empty()) {
        random_suffix_ = generateRandomString(8);
    }
}

std::string Rebuild::generateRandomString(size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
    return result;
}

bool Rebuild::isValidExecutable(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        Logger::logWarning(std::format("Cannot open file for validation: {}", filePath));
        return false;
    }

    char header[2];
    file.read(header, 2);
    file.close();
    bool isValid = (header[0] == 'M' && header[1] == 'Z');
    if (!isValid) {
        Logger::logWarning(std::format("File is not a valid PE executable: {}", filePath));
    }
    return isValid;
}

std::string Rebuild::getCurrentExeName() {
    char exePath[MAX_PATH];
    DWORD pathLength = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (pathLength == 0) {
        Logger::logWarning(std::format("Failed to get module filename: {}", GetLastError()));
        return "ErScripts.exe";
    }

    std::string fullPath(exePath, pathLength);
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return fullPath.substr(lastSlash + 1);
    }
    return fullPath;
}

std::string Rebuild::getCurrentExePath() {
    char exePath[MAX_PATH];
    DWORD pathLength = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (pathLength == 0) {
        Logger::logWarning(std::format("Failed to get module path: {}", GetLastError()));
        return "ErScripts.exe";
    }
    return std::string(exePath, pathLength);
}

bool Rebuild::unpackIfNeeded() {
    std::string current_exe = getCurrentExePath();

    // Read binary
    std::ifstream in_file(current_exe, std::ios::binary);
    if (!in_file) {
        Logger::logWarning(std::format("Failed to read binary for unpacking: {}", current_exe));
        return false;
    }
    std::vector<char> binary_data((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());
    in_file.close();

    // Check for marker (2 bytes: random byte + 0xFF)
    if (binary_data.size() < 2 || binary_data[binary_data.size() - 1] != 0xFF) {
        return true; // Not packed
    }

    // Remove random byte and marker
    binary_data.pop_back(); // Marker (0xFF)
    binary_data.pop_back(); // Random byte

    // Write unpacked binary
    for (int retry = 0; retry < 3; ++retry) {
        std::ofstream out_file(current_exe, std::ios::binary);
        if (out_file) {
            out_file.write(binary_data.data(), binary_data.size());
            out_file.close();
            return true;
        }
        Logger::logWarning(std::format("Retry {}: Failed to write unpacked binary: {}", retry + 1, GetLastError()));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Logger::logWarning("Failed to unpack binary after retries");
    return false;
}

bool Rebuild::createPolymorphicBinary(std::string& output_path) {
    // Get TEMP directory
    char temp_dir[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_dir) == 0) {
        Logger::logWarning(std::format("Failed to get TEMP path: {}", GetLastError()));
        return false;
    }

    // Use stored suffix
    output_path = std::string(temp_dir) + "ErScripts_temp_" + random_suffix_ + ".exe";

    // Check disk space (~10MB)
    ULARGE_INTEGER freeBytes;
    if (GetDiskFreeSpaceExA(temp_dir, &freeBytes, NULL, NULL)) {
        if (freeBytes.QuadPart < 10 * 1024 * 1024) {
            Logger::logWarning("Insufficient disk space for rebuild");
            return false;
        }
    }

    // Read current binary
    std::string current_exe = getCurrentExePath();
    std::ifstream in_file(current_exe, std::ios::binary);
    if (!in_file) {
        Logger::logWarning("Failed to read current binary for rebuild");
        return false;
    }
    std::vector<char> binary_data((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());
    in_file.close();

    // Append one random byte
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> byte_dis(0, 255);
    binary_data.push_back(static_cast<char>(byte_dis(gen)));

    // Append marker (0xFF)
    binary_data.push_back(static_cast<char>(0xFF));

    // Write temp binary
    for (int retry = 0; retry < 3; ++retry) {
        std::ofstream out_file(output_path, std::ios::binary);
        if (out_file) {
            out_file.write(binary_data.data(), binary_data.size());
            out_file.close();
            return true;
        }
        Logger::logWarning(std::format("Retry {}: Failed to write temp binary: {}", retry + 1, GetLastError()));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Logger::logWarning("Failed to write temp binary after retries");
    return false;
}

bool Rebuild::replaceWithNewBinary(const std::string& newFilePath, const std::string& bat_path) {
    if (!isValidExecutable(newFilePath)) {
        Logger::logWarning("New binary is not a valid executable");
        return false;
    }

    std::string current_exe = getCurrentExePath();

    // Write batch file
    for (int retry = 0; retry < 3; ++retry) {
        std::ofstream bat_file(bat_path);
        if (bat_file) {
            bat_file << "@echo off\n";
            bat_file << "timeout /t 1 >nul\n";
            bat_file << "move /Y \"" << newFilePath << "\" \"" << current_exe << "\"\n";
            bat_file << "if exist \"" << current_exe << "\" (\n";
            bat_file << "    start \"\" \"" << current_exe << "\" --run /requestuia\n";
            bat_file << "    del \"%~f0\"\n";
            bat_file << ")\n";
            bat_file.close();
            break;
        }
        Logger::logWarning(std::format("Retry {}: Failed to create batch file: {}", retry + 1, GetLastError()));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!std::filesystem::exists(bat_path)) {
        Logger::logWarning("Failed to create batch file after retries");
        return false;
    }

    // Run batch file
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    sei.lpFile = bat_path.c_str();
    sei.lpVerb = "open";
    sei.nShow = SW_HIDE;
    if (!ShellExecuteExA(&sei)) {
        Logger::logWarning(std::format("Failed to execute batch file: {}", GetLastError()));
        std::filesystem::remove(bat_path);
        return false;
    }

    return true;
}

void Rebuild::cleanupTempFiles() {
    char temp_dir[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_dir) == 0) {
        Logger::logWarning(std::format("Failed to get TEMP path for cleanup: {}", GetLastError()));
        return;
    }

    std::string temp_exe = std::string(temp_dir) + "ErScripts_temp_" + random_suffix_ + ".exe";
    std::string temp_bat = std::string(temp_dir) + "rebuild_" + random_suffix_ + ".bat";

    if (std::filesystem::exists(temp_exe)) {
        std::filesystem::remove(temp_exe);
        Logger::logInfo(std::format("Cleaned up temp file: {}", temp_exe));
    }
    if (std::filesystem::exists(temp_bat)) {
        std::filesystem::remove(temp_bat);
        Logger::logInfo(std::format("Cleaned up batch file: {}", temp_bat));
    }
}

bool Rebuild::rebuildAndRelaunch(bool should_rebuild) {
    if (!should_rebuild) {
        // Unpack for --run
        return unpackIfNeeded();
    }

    // Unpack before rebuild
    if (!unpackIfNeeded()) {
        Logger::logWarning("Failed to unpack binary before rebuild");
        return false;
    }

    // Create new binary
    std::string new_binary;
    if (!createPolymorphicBinary(new_binary)) {
        Logger::logWarning("Failed to create polymorphic binary");
        return false;
    }

    // Create batch file in TEMP
    char temp_dir[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_dir) == 0) {
        Logger::logWarning(std::format("Failed to get TEMP path: {}", GetLastError()));
        std::filesystem::remove(new_binary);
        return false;
    }
    std::string bat_path = std::string(temp_dir) + "rebuild_" + random_suffix_ + ".bat";

    // Replace and relaunch
    if (replaceWithNewBinary(new_binary, bat_path)) {
        Logger::logInfo("Rebuild initiated. Relaunching...");
        return true;
    }
    else {
        Logger::logWarning("Failed to replace binary for rebuild");
        std::filesystem::remove(new_binary);
        if (std::filesystem::exists(bat_path)) {
            std::filesystem::remove(bat_path);
        }
        return false;
    }
}