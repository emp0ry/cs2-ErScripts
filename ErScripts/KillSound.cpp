#include "ErScripts.h"
#include "SimpleSound.h"
#include "SoundsShellcodes.h"

void ErScripts::KillSound() {
    startWorker("KillSound", [this]() {
        SimpleSound sound(hitSound, hitSoundLen);
        const auto initial = Config::snapshot();
        sound.secondBuffer(initial->killSoundFileName);

        std::string lastCustomFile = initial->killSoundFileName;

        int oldKills = globals::localPlayerKills;

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->killSoundState) {
                if (settings->killSoundFileName != lastCustomFile) {
                    sound.secondBuffer(settings->killSoundFileName);

                    lastCustomFile = settings->killSoundFileName;
                }

                if (globals::localPlayerKills != oldKills) {
                    if (globals::localPlayerKills > oldKills && globals::localPlayerKills) {
                        sound.setVolume(settings->killSoundVolume);
                        sound.play();
                    }
                    oldKills = globals::localPlayerKills;
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
