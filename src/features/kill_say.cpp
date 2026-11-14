#include "core/config.hpp"
#include "core/globals.hpp"
#include "features/features.hpp"
#include "game/game_console.hpp"

#include <format>

void Features::killSay() {
    startWorker("KillSay", [this]() {
        auto previous = globals::getGameState();
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto game = globals::getGameState();
            if (settings->killSayState) {
                if (game.inGame && previous.inGame && game.session == previous.session && previous.kills >= 0 &&
                    game.kills > previous.kills)
                    console_.send(8, std::format("say {}", settings->killSayText));
            }

            previous = game;
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
