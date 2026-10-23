#include "GSIServer.h"
#include "Updater.h"
#include "UIAccess.h"
#include "Rebuild.h"
#include "ErScripts.h"
#include "Overlay.h"

#include "AppVersion.h"
#include "ConfigExport.h"

int main(int argc, char *argv[]) {
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
    if (preview) {
        globals::previewMode = true;
        globals::width = 1280;
        globals::height = 900;
        crosshair::Settings settings;
        settings.modernFields = 15;
        settings.loaded = true;
        globals::setConfig(settings);
        SetConsoleTitleA("ErScripts preview");
        Overlay overlay;
        overlay.run();
        while (!globals::finish)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 0;
    }

    if (!IsDebuggerPresent()) {

        CreateMutexA(0, FALSE, "Local\\erscripts");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {

            Updater::cleanupTempFiles();
            MessageBoxA(NULL, "ErScripts is already running!", 0, MB_OK);
            return -1;
        }

        if (checkUpdates) {
            Updater updater(APP_VERSION, "emp0ry", "cs2-ErScripts", "ErScripts");
            if (updater.checkAndUpdate())
                return 0;
        }

        DWORD dwErr = PrepareForUIAccess();
        if (dwErr != ERROR_SUCCESS) {
            Logger::logWarning(std::format("Failed to prepare for UI Access: {}", dwErr));
            std::cout << "[*] Press Enter to exit...";
            std::cin.get();
            return -1;
        }
    }

    SetConsoleTitleA(std::format("ErScripts {}", APP_VERSION).c_str());

    std::cout << "[-] *---------------------------------------*" << std::endl;
    std::cout << "[>] |  ErScripts by emp0ry                  |" << std::endl;
    std::cout << "[>] |  GitHub - https://github.com/emp0ry/  |" << std::endl;
    std::cout << "[-] *---------------------------------------*" << std::endl;

    cfg->load("default");
    gradient.setConfig(cfg->gradient);

    ErScripts es;
    es.Initalization();

    Overlay overlay;
    overlay.run();

    GSIServer gsi;
    gsi.run();

    es.ConsoleLogStream();
    es.AutoAccept();
    es.BombTimer();
    es.KillSound();
    es.RoundStartAlert();
    es.InitBinds();
    es.PixelTrigger();
    es.Crosshair();
    es.RGBCrosshair();
    es.KnifeSwitch();
    es.AutoPistol();
    es.AntiAfk();
    es.CS2Binds();
    es.KillSay();
    es.AutoStop();
    es.ChatSpammer();

    while (!globals::finish) {
        static int prevExitBind = cfg->erScriptsExitBind;
        static std::chrono::steady_clock::time_point changeTime;
        static bool isDelayActive = false;

        if (prevExitBind != cfg->erScriptsExitBind) {
            prevExitBind = cfg->erScriptsExitBind;
            changeTime = std::chrono::steady_clock::now();
            isDelayActive = true;
        } else if (isDelayActive) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - changeTime).count();
            if (elapsed >= 2000) {
                isDelayActive = false;
            }
        } else {
            if (GetAsyncKeyState(cfg->erScriptsExitBind) & 0x8000)
                globals::finish = true;
        }

        auto &exporter = ConfigExport::instance();
        exporter.flush();
        if (!exporter.status().busy && ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
            static auto lastPing = std::chrono::steady_clock::now();
            static auto lastSettings = lastPing;
            const auto now = std::chrono::steady_clock::now();
            if (cfg->watermarkState && now - lastPing >= std::chrono::seconds(cfg->watermarkPingUpdateRate)) {
                ErScripts::CommandsSender(5, "sys_info");
                lastPing = now;
            }
            if (now - lastSettings >= std::chrono::milliseconds(1500)) {
                ErScripts::CommandsSender(5, "print_changed_convars; echo [ErScripts] convars_end");
                lastSettings = now;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    gsi.stop();

    exit(0);
}
