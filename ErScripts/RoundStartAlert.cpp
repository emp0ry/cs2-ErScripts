#include "CS2Functions.h"
#include "SimpleSound.h"
#include "SoundsShellcodes.h"

void CS2Functions::RoundStartAlert() {
    std::thread([this]() {
        SimpleSound sound(alertSound, alertSoundLen);

        bool oldRoundStartState = globals::roundStartState;

        while (!globals::finish) {
            if (cfg->roundStartAlertState) {
                if (!CS2Functions::GetWindowState()) {
                    if (globals::roundStartState != oldRoundStartState) {
                        oldRoundStartState = globals::roundStartState;

                        if (globals::roundStartState) {
                            sound.setVolume(cfg->roundStartAlertVolume);
                            sound.play();
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        sound.~SimpleSound();
    }).detach();
}