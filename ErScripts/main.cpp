#include "GSIServer.h"
#include "Updater.h"
#include "UIAccess.h"
#include "Rebuild.h"
#include "ErScripts.h"
#include "Overlay.h"

#define APP_VERSION "1.1.8"

int main(int argc, char* argv[]) {
    Logger::EnableANSIColors();

    if (!IsDebuggerPresent()) {
		/* Rebuild */
        bool should_rebuild = true;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--run") {
                should_rebuild = false;
                break;
            }
        }

        // Check if program already running
        CreateMutexA(0, FALSE, "Local\\erscripts");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            Rebuild::cleanupTempFiles();
            Updater::cleanupTempFiles();
            MessageBoxA(NULL, "ErScripts is already running!", 0, MB_OK);
            return -1;
        }

        // Rebuild or unpack
        Rebuild rebuilder;
        if (!rebuilder.rebuildAndRelaunch(should_rebuild)) {
            std::cout << "[*] Rebuild or unpack failed. Continuing with current binary.\n";
        }
        if (should_rebuild) {
            return 0; // Exit after relaunch
        }

        /* Auto updater */
        Updater updater(APP_VERSION, "emp0ry", "cs2-ErScripts", "ErScripts");
        if (updater.checkAndUpdate())
            return 0;

        /* Ui Access */
        DWORD dwErr = PrepareForUIAccess();
        if (dwErr != ERROR_SUCCESS) {
            Logger::logWarning(std::format("Failed to prepare for UI Access: {}", dwErr));
            std::cout << "[*] Press Enter to exit...";
            std::cin.get();
            return -1;
        }
    }

    AllocConsole();
    freopen("CONOUT$", "w", stdout);

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
    es.InitBinds();

    es.AutoAccept();
    es.PixelTrigger();
    es.Crosshair();
    es.BombTimer();
    es.RGBCrosshair();
    es.KnifeSwitch();
    es.AutoPistol();
    es.AntiAfk();
    es.CS2Binds();
    es.KillSay();
    es.KillSound();
    es.RoundStartAlert();
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
        }
        else if (isDelayActive) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - changeTime).count();
            if (elapsed >= 2000) {
                isDelayActive = false;
            }
        }
        else {
            if (GetAsyncKeyState(cfg->erScriptsExitBind) & 0x8000) globals::finish = true;
        }

        /* Update Ping */
        if (ErScripts::GetWindowState && ErScripts::GetCursorState()) {
            static auto lastUpdate = std::chrono::steady_clock::now();

            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed = now - lastUpdate;

            if (cfg->watermarkState) {
                if (elapsed.count() >= cfg->watermarkPingUpdateRate) {
                    ErScripts::CommandsSender(5, "sys_info;clear");
                    lastUpdate = now;
                }
            }

            if (cfg->sniperCrosshairState || cfg->recoilCrosshairState || cfg->angleBindState) {
                if (elapsed.count() >= 1.5f) {
                    ErScripts::CommandsSender(5, "print_changed_convars; clear; echo *---------------------------------------*; echo |  ErScripts by emp0ry                  |; echo |  GitHub - github.com/emp0ry/          |; echo *---------------------------------------*");
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    gsi.stop();

	exit(0);
}