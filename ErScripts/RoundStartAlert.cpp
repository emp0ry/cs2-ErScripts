#include "ErScripts.h"
#include "SimpleSound.h"
#include "SoundsShellcodes.h"

void ErScripts::RoundStartAlert() {
    startWorker("RoundStartAlert", [this]() {
        SimpleSound sound(alertSound, alertSoundLen);
        const auto initial = Config::snapshot();
        sound.secondBuffer(initial->roundStartAlertFileName);

        std::string lastCustomFile = initial->roundStartAlertFileName;

        bool oldRoundStartState = globals::roundStartState;

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->roundStartAlertState) {
                if (settings->roundStartAlertFileName != lastCustomFile) {
                    sound.secondBuffer(settings->roundStartAlertFileName);

                    lastCustomFile = settings->roundStartAlertFileName;
                }

                if (globals::roundStartState != oldRoundStartState) {
                    oldRoundStartState = globals::roundStartState;

                    if (globals::roundStartState) {
                        if (!ErScripts::GetWindowState()) {
                            sound.setVolume(settings->roundStartAlertVolume);
                            sound.play();
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
