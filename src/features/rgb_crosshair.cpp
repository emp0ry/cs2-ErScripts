#include "core/color_gradient.hpp"
#include "core/config.hpp"
#include "core/globals.hpp"
#include "features/features.hpp"
#include "game/game_console.hpp"

#include <format>

void Features::rgbCrosshair() {
    startWorker("RGBCrosshair", [this]() {
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->rgbCrosshairState) {
                RgbColor color = colorGradient().getCurrentColor();
                console_.send(2, std::format("cl_crosshaircolor_r {}; cl_crosshaircolor_g {}; cl_crosshaircolor_b {}",
                                             color.r, color.g, color.b));
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
