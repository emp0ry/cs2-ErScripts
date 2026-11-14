#include "features/Features.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "game/GameWindow.h"
#include "game/GameConsole.h"
#include <cstdlib>

void Features::AutoPistol() {
    startWorker("AutoPistol", [this]() {
        bool releasePending = false;
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto game = globals::getGameState();
            if (releasePending)
                releasePending = !console.send(6, "attack -999 1 0");
            if (!releasePending && settings->autoPistolState && game.alive && game.pistol && !game.revolver &&
                !game.cz75) {
                if (game::hasFocus() && game::cursorHidden()) {
                    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
                        if (console.send(6, "attack 1 1 0")) {
                            releasePending = true;
                            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                            releasePending = !console.send(6, "attack -999 1 0");
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
        if (releasePending)
            console.send(6, "attack -999 1 0");
    });
}
