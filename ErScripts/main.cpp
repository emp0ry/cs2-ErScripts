#include "GSIServer.h"
#include "Updater.h"
#include "UIAccess.h"
#include "ErScripts.h"
#include "Overlay.h"

int main() {
    if (!IsDebuggerPresent()) {
        /* Auto updater */
        Updater updater("1.0.6", "emp0ry", "cs2-ErScripts", "ErScripts");
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

    Logger::EnableANSIColors();

    /* Check if program already running */
    CreateMutexA(0, FALSE, "Local\\erscripts");   // Try to create a named mutex
    if (GetLastError() == ERROR_ALREADY_EXISTS) { // Did the mutex already exist?
        MessageBoxA(NULL, "ErScripts is already running!", 0, MB_OK);
        return -1;
    }

    std::cout << "[-] *---------------------------------------*" << std::endl;
    std::cout << "[>] |  ErScripts by emp0ry                  |" << std::endl;
    std::cout << "[>] |  Github - https://github.com/emp0ry/  |" << std::endl;
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
    es.SnapTap();

    while (!globals::finish) {
        if (GetAsyncKeyState(VK_END) & 0x8000) globals::finish = true;
        
        /* Update Ping */
        if (cfg->watermarkState) {
            if (ErScripts::GetWindowState && ErScripts::GetCursorState()) {
                static auto lastUpdate = std::chrono::steady_clock::now();

                auto now = std::chrono::steady_clock::now();
                std::chrono::duration<float> elapsed = now - lastUpdate;

                if (elapsed.count() >= cfg->watermarkPingUpdateRate) {
                    ErScripts::CommandsSender(5, "sys_info");
                    lastUpdate = now; // Reset the timer
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    gsi.stop();

	exit(0);
}