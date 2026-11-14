#include "features/Features.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "game/GameWindow.h"
#include "game/GameConsole.h"

void Features::KnifeSwitch() {
    startWorker("KnifeSwitch", [this]() {
        bool oldKnifeState = false;
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto game = globals::getGameState();
            if (settings->knifeSwitchState && game.connected) {
                if (game::hasFocus() && game::cursorHidden()) {
                    if (game.knife != oldKnifeState) {
                        if (console.send(4, "switchhands"))
                            oldKnifeState = game.knife;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
