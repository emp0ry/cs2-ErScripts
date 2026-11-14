#pragma once

#include <string>
#include <iostream>
#include <format>
#include <windows.h>

class Logger {
  public:
    static void EnableANSIColors();

    static void logWarning(std::string_view message);
    static void logInfo(std::string_view message);
    static void logSuccess(std::string_view message);

    static void logWarning(std::wstring_view message);
    static void logInfo(std::wstring_view message);
    static void logSuccess(std::wstring_view message);
};
