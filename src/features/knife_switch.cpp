#include "core/config.hpp"
#include "core/globals.hpp"
#include "features/features.hpp"
#include "game/game_console.hpp"
#include "game/game_window.hpp"

void Features::knifeSwitch() {
    startWorker("KnifeSwitch", [this]() {
        bool oldKnifeState = false;
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto game = globals::getGameState();
            if (settings->knifeSwitchState && game.connected) {
                if (game::hasFocus() && game::cursorHidden()) {
                    if (game.knife != oldKnifeState) {
                        if (console_.send(4, "switchhands"))
                            oldKnifeState = game.knife;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
