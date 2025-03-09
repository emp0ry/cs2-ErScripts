#include "GSIServer.h"
#include "UIAccess.h"
#include "CS2Functions.h"
#include "Overlay.h"

int main() {
    /* Ui Access */
    DWORD dwErr = PrepareForUIAccess();
    if (dwErr != ERROR_SUCCESS) {
        Logger::logWarning(std::format("Failed to prepare for UI Access: {}", dwErr));
        std::cout << "[*] Press Enter to exit...";
        std::cin.get();
        return -1;
    }

    /* Check if program already running */
    CreateMutexA(0, FALSE, "Local\\erscripts");   // Try to create a named mutex
    if (GetLastError() == ERROR_ALREADY_EXISTS) { // Did the mutex already exist?
        MessageBoxA(NULL, "ErScripts is already running!", 0, MB_OK);
        return -1;
    }

    Logger::EnableANSIColors();

    std::cout << "[-] *---------------------------------------*" << std::endl;
    std::cout << "[>] |  ErScripts by emp0ry                  |" << std::endl;
    std::cout << "[>] |  Github - https://github.com/emp0ry/  |" << std::endl;
    std::cout << "[-] *---------------------------------------*" << std::endl;

    cfg->load("default");
    gradient.setConfig(cfg->gradient);

    CS2Functions cs2;
    cs2.Initalization();

    Overlay overlay;
    overlay.run();

    GSIServer gsi;
    gsi.run();

    cs2.ConsoleLogStream();
    cs2.InitBinds();

    cs2.AutoAccept();
    cs2.PixelTrigger();
    cs2.Crosshair();
    cs2.BombTimer();
    cs2.RGBCrosshair();
    cs2.KnifeSwitch();
    cs2.AutoPistol();
    cs2.AntiAfk();
    cs2.CS2Binds();

    while (!globals::finish) {
        if (GetAsyncKeyState(VK_END) & 0x8000) globals::finish = true;
        
        /* Update Ping */
        if (cfg->watermarkState) {
            if (CS2Functions::GetWindowState && CS2Functions::GetCursorState()) {
                static auto lastUpdate = std::chrono::steady_clock::now();

                auto now = std::chrono::steady_clock::now();
                std::chrono::duration<float> elapsed = now - lastUpdate;

                if (elapsed.count() >= cfg->watermarkPingUpdateRate) {
                    CS2Functions::CommandsSender(5, "sys_info");
                    lastUpdate = now; // Reset the timer
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    gsi.stop();

	exit(0);
}