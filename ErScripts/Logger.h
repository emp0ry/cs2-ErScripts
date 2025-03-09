#pragma once

#include <string>
#include <iostream>
#include <format>
#include <windows.h>

class Logger {
public:
    // Enable ANSI color support
    static void EnableANSIColors();

    // Overloads for narrow strings (std::string)
    static void logWarning(std::string_view message);
    static void logInfo(std::string_view message);
    static void logSuccess(std::string_view message);

    // Overloads for wide strings (std::wstring)
    static void logWarning(std::wstring_view message);
    static void logInfo(std::wstring_view message);
    static void logSuccess(std::wstring_view message);
};