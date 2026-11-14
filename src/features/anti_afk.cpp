#include "core/config.hpp"
#include "core/globals.hpp"
#include "features/features.hpp"
#include "game/game_window.hpp"
#include "platform/input.hpp"

#include <cstdlib>

void Features::antiAfk() {
    startWorker("AntiAfk", [this]() {
        bool oldRoundStartState = false;
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto roundStarted = globals::getGameState().roundLive;
            if (settings->antiAfkState) {
                if (game::hasFocus() && game::cursorHidden()) {
                    if (roundStarted != oldRoundStartState) {

                        if (roundStarted) {
                            if (!(GetAsyncKeyState('W') & 0x8000)) {
                                input::KeyPress key('W');
                                if (key.active())
                                    std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 3) + 16));
                            }
                        }
                    }
                }
            }

            oldRoundStartState = roundStarted;
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
