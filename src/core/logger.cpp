#include "core/logger.hpp"

#include <syncstream>

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_WHITE "\033[37m"
#define COLOR_RESET "\033[0m"

namespace {
    std::string toUtf8(std::wstring_view wstr) {
        if (wstr.empty())
            return {};
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string result(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), sizeNeeded, nullptr, nullptr);
        return result;
    }
} // namespace

void Logger::logWarning(std::string_view message) {
    std::osyncstream(std::cerr) << std::format("{}[!] {}{}\n", COLOR_RED, message, COLOR_RESET);
}

void Logger::logInfo(std::string_view message) {
    std::osyncstream(std::cout) << std::format("{}[*] {}{}\n", COLOR_WHITE, message, COLOR_RESET);
}

void Logger::logSuccess(std::string_view message) {
    std::osyncstream(std::cout) << std::format("{}[+] {}{}\n", COLOR_GREEN, message, COLOR_RESET);
}

void Logger::logWarning(std::wstring_view message) {
    logWarning(toUtf8(message));
}

void Logger::logInfo(std::wstring_view message) {
    logInfo(toUtf8(message));
}

void Logger::logSuccess(std::wstring_view message) {
    logSuccess(toUtf8(message));
}

void Logger::enableAnsiColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
