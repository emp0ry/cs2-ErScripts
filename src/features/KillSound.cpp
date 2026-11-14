#include "features/Features.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "platform/SimpleSound.h"
#include "platform/SoundsShellcodes.h"

void Features::KillSound() {
    startWorker("KillSound", [this]() {
        SimpleSound sound(hitSound, hitSoundLen);
        const auto initial = Config::snapshot();
        sound.secondBuffer(initial->killSoundFileName);

        std::string lastCustomFile = initial->killSoundFileName;

        auto previous = globals::getGameState();

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto game = globals::getGameState();
            if (settings->killSoundState) {
                if (settings->killSoundFileName != lastCustomFile) {
                    sound.secondBuffer(settings->killSoundFileName);

                    lastCustomFile = settings->killSoundFileName;
                }

                if (game.inGame && previous.inGame && game.session == previous.session && previous.kills >= 0 &&
                    game.kills > previous.kills) {
                    sound.setVolume(settings->killSoundVolume);
                    sound.play();
                }
            }

            previous = game;
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
