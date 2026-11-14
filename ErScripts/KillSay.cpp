#include "ErScripts.h"

void ErScripts::KillSay() {
    startWorker("KillSay", [this]() {
        int oldKills = globals::localPlayerKills;
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->killSayState) {
                if (globals::localPlayerKills != oldKills) {
                    if (globals::localPlayerKills > oldKills)
                        CommandsSender(8, std::format("say {}", settings->killSayText));
                    oldKills = globals::localPlayerKills;
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
