#include "features/Features.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "game/GameConsole.h"
#include <format>

void Features::KillSay() {
    startWorker("KillSay", [this]() {
        auto previous = globals::getGameState();
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto game = globals::getGameState();
            if (settings->killSayState) {
                if (game.inGame && previous.inGame && game.session == previous.session && previous.kills >= 0 &&
                    game.kills > previous.kills)
                    console.send(8, std::format("say {}", settings->killSayText));
            }

            previous = game;
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
