#include "ErScripts.h"

void ErScripts::RGBCrosshair() {
    std::thread([this]() {
        while (!globals::finish) {
            if (cfg->rgbCrosshairState) {
                RGBColor color = gradient.getCurrentColor();
                CommandsSender(2, std::format("cl_crosshaircolor 5; cl_crosshaircolor_r {}; cl_crosshaircolor_g {}; cl_crosshaircolor_b {}", color.r, color.g, color.b));
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    }).detach();
}