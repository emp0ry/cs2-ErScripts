#include "ErScripts.h"

void ErScripts::RGBCrosshair() {
    startWorker("RGBCrosshair", [this]() {
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->rgbCrosshairState) {
                RGBColor color = gradient.getCurrentColor();
                CommandsSender(2, std::format("cl_crosshaircolor_r {}; cl_crosshaircolor_g {}; cl_crosshaircolor_b {}",
                                              color.r, color.g, color.b));
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
