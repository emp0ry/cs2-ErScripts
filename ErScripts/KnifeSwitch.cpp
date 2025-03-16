#include "ErScripts.h"

void ErScripts::KnifeSwitch() {
    std::thread([this]() {
        bool oldKnifeState = false;
        while (!globals::finish) {
            if (cfg->knifeSwitchState) {
                if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
                    if (globals::knifeState != oldKnifeState) {
                        CommandsSender(4, "switchhands");
                        oldKnifeState = globals::knifeState;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }).detach();
}