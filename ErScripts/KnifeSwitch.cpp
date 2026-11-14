#include "ErScripts.h"

void ErScripts::KnifeSwitch() {
    startWorker("KnifeSwitch", [this]() {
        bool oldKnifeState = false;
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->knifeSwitchState) {
                if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
                    if (globals::knifeState != oldKnifeState) {
                        CommandsSender(4, "switchhands");
                        oldKnifeState = globals::knifeState;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
