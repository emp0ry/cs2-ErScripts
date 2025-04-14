#include "ErScripts.h"

void ErScripts::AutoPistol() {
    std::thread([]() {
        while (!globals::finish) {
            if (cfg->autoPistolState && globals::pistolState && !globals::revolverState && !globals::cz75aState) {
                if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
                    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                        ErScripts::CommandsSender(6, "attack 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(6, "attack -999 1 0");
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    }).detach();
}