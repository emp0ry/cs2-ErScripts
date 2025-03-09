#include "Logger.h"

#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_RESET   "\033[0m"

namespace {
    std::string to_utf8(std::wstring_view wstr) {
        if (wstr.empty()) return {};
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), size_needed, nullptr, nullptr);
        return result;
    }
}

// Narrow string implementations
void Logger::logWarning(std::string_view message) {
    std::cerr << std::format("{}[!] {}{}\n", COLOR_RED, message, COLOR_RESET);
}

void Logger::logInfo(std::string_view message) {
    std::cout << std::format("{}[*] {}{}\n", COLOR_WHITE, message, COLOR_RESET);
}

void Logger::logSuccess(std::string_view message) {
    std::cout << std::format("{}[+] {}{}\n", COLOR_GREEN, message, COLOR_RESET);
}

// Wide string implementations (convert to UTF-8 and call narrow overloads)
void Logger::logWarning(std::wstring_view message) {
    logWarning(to_utf8(message));
}

void Logger::logInfo(std::wstring_view message) {
    logInfo(to_utf8(message));
}

void Logger::logSuccess(std::wstring_view message) {
    logSuccess(to_utf8(message));
}

void Logger::EnableANSIColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}