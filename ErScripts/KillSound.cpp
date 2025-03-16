#include "CS2Functions.h"
#include "SimpleSound.h"
#include "SoundsShellcodes.h"

void CS2Functions::KillSound() {
    std::thread([this]() {
        SimpleSound sound(hitSound, hitSoundLen);
        sound.secondBuffer(cfg->killSoundFileName);

        std::string lastCustomFile = cfg->killSoundFileName;

        int oldKills = globals::localPlayerKills;

        while (!globals::finish) {
            if (cfg->killSoundState) {
                if (cfg->killSoundFileName != lastCustomFile) {
                    sound.secondBuffer(cfg->killSoundFileName);

                    lastCustomFile = cfg->killSoundFileName;
                }

                if (globals::localPlayerKills != oldKills) {
                    if (globals::localPlayerKills > oldKills && globals::localPlayerKills) {
                        sound.setVolume(cfg->killSoundVolume);
                        sound.play();
                    }
                    oldKills = globals::localPlayerKills;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        sound.~SimpleSound();
    }).detach();
}