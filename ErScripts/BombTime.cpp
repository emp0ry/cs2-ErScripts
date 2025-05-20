#include "ErScripts.h"

void ErScripts::BombTimer() {
    std::thread([this]() {
        while (!globals::finish) {
            if (cfg->bombTimerState) {
                static bool isbombTimerStarted = false;
                static std::chrono::system_clock::time_point bombPlantedTime;

                if (globals::bombState) {
                    if (!isbombTimerStarted) {
                        bombPlantedTime = std::chrono::system_clock::now();
                        isbombTimerStarted = true;
                    }

                    if (isbombTimerStarted) {
                        globals::bombTime = 40000 - std::chrono::duration<double, std::milli>(std::chrono::system_clock::now() - bombPlantedTime).count();
                        if (globals::bombTime <= 0) globals::bombTime = 0.0l;
                    }
                }
                else {
                    globals::bombTime = 0.0l;
                    isbombTimerStarted = false;
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    }).detach();
}