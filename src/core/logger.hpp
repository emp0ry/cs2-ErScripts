#pragma once

#include <format>
#include <iostream>
#include <string>
#include <windows.h>

class Logger {
  public:
    static void enableAnsiColors();

    static void logWarning(std::string_view message);
    static void logInfo(std::string_view message);
    static void logSuccess(std::string_view message);

    static void logWarning(std::wstring_view message);
    static void logInfo(std::wstring_view message);
    static void logSuccess(std::wstring_view message);
};
