#include "ErScripts.h"

void ErScripts::Crosshair() {
    std::thread([this]() {
        std::vector<int> blackColor = {0, 0, 0};
        bool oldRecoilCrosshairState = false;

        while (!globals::finish) {
            globals::flushCrosshair();
            if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {

                if (globals::getPlayerState().sniper()) {
                    std::vector<int> pixelColor1 = GetPixelColor(globals::posX + 1, globals::posY + 1);
                    std::vector<int> pixelColor2 =
                        GetPixelColor(globals::posX + 1, globals::posY + globals::height - 6);

                    globals::isScope =
                        (isColorSimilar(pixelColor1, blackColor, 20) && isColorSimilar(pixelColor2, blackColor, 20));
                } else {
                    globals::isScope = false;
                }

                if (cfg->recoilCrosshairState != oldRecoilCrosshairState) {
                    if (cfg->recoilCrosshairState) {
                        CommandsSender(3, "cl_crosshair_recoil true");
                    } else {
                        CommandsSender(3, "cl_crosshair_recoil false");
                    }

                    oldRecoilCrosshairState = cfg->recoilCrosshairState;
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    }).detach();
}
