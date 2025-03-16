#include "CS2Functions.h"

void CS2Functions::AntiAfk() {
    std::thread([]() {
        bool oldRoundStartState = false;
        while (!globals::finish) {
            if (cfg->antiAfkState) {
                if (CS2Functions::GetWindowState() && CS2Functions::GetCursorState()) {
                    if (globals::roundStartState != oldRoundStartState) {
                        oldRoundStartState = globals::roundStartState;

                        if (globals::roundStartState) {
                            if (!(GetAsyncKeyState('W') & 0x8000)) {
                                Keyboard('W', true, false);
                                std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 3) + 16));
                                Keyboard('W', false, false);
                            }
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }).detach();
}