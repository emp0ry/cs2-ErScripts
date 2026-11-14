#include "core/config.hpp"
#include "core/globals.hpp"
#include "features/features.hpp"
#include "game/game_window.hpp"
#include "platform/simple_sound.hpp"
#include "platform/sound_data.hpp"

void Features::roundStartAlert() {
    startWorker("RoundStartAlert", [this]() {
        SimpleSound sound(alertSound, alertSoundLen);
        const auto initial = Config::snapshot();
        sound.secondBuffer(initial->roundStartAlertFileName);

        std::string lastCustomFile = initial->roundStartAlertFileName;

        bool oldRoundStartState = globals::getGameState().roundLive;

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto roundStarted = globals::getGameState().roundLive;
            if (settings->roundStartAlertState) {
                if (settings->roundStartAlertFileName != lastCustomFile) {
                    sound.secondBuffer(settings->roundStartAlertFileName);

                    lastCustomFile = settings->roundStartAlertFileName;
                }

                if (roundStarted != oldRoundStartState) {

                    if (roundStarted) {
                        if (!game::hasFocus()) {
                            sound.setVolume(settings->roundStartAlertVolume);
                            sound.play();
                        }
                    }
                }
            }

            oldRoundStartState = roundStarted;
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
