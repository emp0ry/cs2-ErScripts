#include "update/Updater.h"
#include "platform/UIAccess.h"
#include "app/Application.h"
#include <backends/imgui_impl_win32.h>

#include "app/AppVersion.h"
#include "core/Logger.h"
#include "core/Globals.h"
#include <iostream>

int main(int argc, char *argv[]) try {
    if (const auto result = Updater::applyPendingUpdate())
        return *result;
    Updater::completeRestart();
    const auto closeHandle = [](HANDLE handle) {
        if (handle)
            CloseHandle(handle);
    };
    std::unique_ptr<void, decltype(closeHandle)> instanceMutex(nullptr, closeHandle);
    ImGui_ImplWin32_EnableDpiAwareness();
    bool preview = false, checkUpdates = true;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--preview")
            preview = true;
        if (argument == "--no-update")
            checkUpdates = false;
    }
    Logger::EnableANSIColors();
    if (!preview && !IsDebuggerPresent()) {

        instanceMutex.reset(CreateMutexA(nullptr, FALSE, "Local\\erscripts"));
        if (GetLastError() == ERROR_ALREADY_EXISTS) {

            MessageBoxA(NULL, "ErScripts is already running!", 0, MB_OK);
            return -1;
        }
        if (!instanceMutex) {
            Logger::logWarning("Cannot create the application instance mutex");
            return -1;
        }

        if (checkUpdates) {
            Updater updater(APP_VERSION, "emp0ry", "cs2-ErScripts", "ErScripts");
            if (updater.checkAndUpdate())
                return 0;
        }

#ifndef _DEBUG
        DWORD dwErr = PrepareForUIAccess();
        if (dwErr != ERROR_SUCCESS) {
            Logger::logWarning(std::format("Failed to prepare for UI Access: {}", dwErr));
            std::cout << "[*] Press Enter to exit...";
            std::cin.get();
            return -1;
        }
#endif
    }

    return Application{}.run(preview);
} catch (const std::exception &error) {
    globals::finish = true;
    Logger::logWarning(std::format("Application failed: {}", error.what()));
    return 1;
}
