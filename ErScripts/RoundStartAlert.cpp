#include "ErScripts.h"
#include "SimpleSound.h"
#include "SoundsShellcodes.h"

void ErScripts::RoundStartAlert() {
    std::thread([this]() {
        SimpleSound sound(alertSound, alertSoundLen);
        sound.secondBuffer(cfg->roundStartAlertFileName);

        std::string lastCustomFile = cfg->roundStartAlertFileName;

        bool oldRoundStartState = globals::roundStartState;

        while (!globals::finish) {
            if (cfg->roundStartAlertState) {
                if (cfg->roundStartAlertFileName != lastCustomFile) {
                    sound.secondBuffer(cfg->roundStartAlertFileName);

                    lastCustomFile = cfg->roundStartAlertFileName;
                }

                if (globals::roundStartState != oldRoundStartState) {
                    oldRoundStartState = globals::roundStartState;

                    if (globals::roundStartState) {
                        if (!ErScripts::GetWindowState()) {
                            sound.setVolume(cfg->roundStartAlertVolume);
                            sound.play();
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    }).detach();
}