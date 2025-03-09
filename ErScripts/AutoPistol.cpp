#include "CS2Functions.h"

void CS2Functions::AutoPistol() {
    std::thread([]() {
        while (!globals::finish) {
            if (cfg->autoPistolState && globals::pistolState) {
                if (CS2Functions::GetWindowState() && CS2Functions::GetCursorState()) {
                    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                        CS2Functions::CommandsSender(6, "attack 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        CS2Functions::CommandsSender(6, "attack -999 1 0");
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }).detach();
}