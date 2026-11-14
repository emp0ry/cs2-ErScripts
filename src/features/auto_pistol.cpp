#include "core/config.hpp"
#include "core/globals.hpp"
#include "features/features.hpp"
#include "game/game_console.hpp"
#include "game/game_window.hpp"

#include <cstdlib>

void Features::autoPistol() {
    startWorker("AutoPistol", [this]() {
        bool releasePending = false;
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto game = globals::getGameState();
            if (releasePending)
                releasePending = !console_.send(6, "attack -999 1 0");
            if (!releasePending && settings->autoPistolState && game.alive && game.pistol && !game.revolver &&
                !game.cz75) {
                if (game::hasFocus() && game::cursorHidden()) {
                    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                        if (console_.send(6, "attack 1 1 0")) {
                            releasePending = true;
                            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                            releasePending = !console_.send(6, "attack -999 1 0");
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
        if (releasePending)
            console_.send(6, "attack -999 1 0");
    });
}
