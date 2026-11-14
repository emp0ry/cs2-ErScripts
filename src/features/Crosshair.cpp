#include "features/Features.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "game/GameWindow.h"
#include "game/GameConsole.h"

void Features::Crosshair() {
    startWorker("Crosshair", [this]() {
        const std::array<int, 3> blackColor = {0, 0, 0};
        bool oldRecoilCrosshairState = false;

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            const auto bounds = globals::getWindowBounds();
            globals::flushCrosshair(console);
            if (game::hasFocus() && game::cursorHidden()) {

                if (globals::getPlayerState().sniper()) {
                    const auto pixelColor1 = GetPixelColor(bounds.x + 1, bounds.y + 1);
                    const auto pixelColor2 = GetPixelColor(bounds.x + 1, bounds.y + bounds.height - 6);

                    globals::isScope = (pixelColor1 && pixelColor2 && isColorSimilar(*pixelColor1, blackColor, 20) &&
                                        isColorSimilar(*pixelColor2, blackColor, 20));
                } else {
                    globals::isScope = false;
                }

                if (settings->recoilCrosshairState != oldRecoilCrosshairState) {
                    if (settings->recoilCrosshairState) {
                        console.send(3, "cl_crosshair_recoil true");
                    } else {
                        console.send(3, "cl_crosshair_recoil false");
                    }

                    oldRecoilCrosshairState = settings->recoilCrosshairState;
                }
            } else {
                globals::isScope = false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
