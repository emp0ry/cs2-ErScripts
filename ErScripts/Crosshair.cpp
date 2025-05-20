#include "ErScripts.h"

void ErScripts::Crosshair() {
    std::thread([this]() {
        std::vector<int> blackColor = { 0, 0, 0 };
        bool oldRecoilCrosshairState = false;

        while (!globals::finish) {
            if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {

                // No longer needed because the print_changed_convars command is used for it
                /*if (globals::configUpdaterState) {
                    ErScripts::CommandsSender("host_writeconfig");
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    Logger::logInfo("Crosshair Updater");
                    globals::crosshair = SteamTools::getCrosshairSettings("730");
                    SteamTools::printCrosshairSettings(*globals::crosshair);
                    globals::configUpdaterState = false;
                }*/

                if (globals::sniperCrosshairState) {
                    std::vector<int> pixelColor1 = GetPixelColor(globals::posX + 1, globals::posY + 1);
                    std::vector<int> pixelColor2 = GetPixelColor(globals::posX + 1, globals::posY + globals::height - 6);

                    //Logger::logInfo(std::format("Color1: RGB({} {} {}) POS({} {}), Color2: RGB({} {} {}) POS({} {})", pixelColor1[0], pixelColor1[1], pixelColor1[2], globals::posX + globals::width / 2, globals::posY, pixelColor2[0], pixelColor2[1], pixelColor2[2], globals::posX + globals::width / 2, globals::posY + globals::height - 1));

                    globals::isScope = (isColorSimilar(pixelColor1, blackColor, 20) && isColorSimilar(pixelColor2, blackColor, 20));
                }

                if (cfg->recoilCrosshairState != oldRecoilCrosshairState) {
                    if (cfg->recoilCrosshairState) {
                        CommandsSender(3, "cl_crosshair_recoil true");
                    }
                    else {
                        CommandsSender(3, "cl_crosshair_recoil false");
                    }
                
                    oldRecoilCrosshairState = cfg->recoilCrosshairState;
                }

                //static bool wasPressed = false;

                //if (cfg->recoilCrosshairState) {
                //    bool isPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000);

                //    // If just pressed
                //    if (isPressed && !wasPressed) {
                //        ErScripts::CommandsSender(3,
                //            "cl_crosshairgap -5; cl_crosshair_outlinethickness 1; \
                //            cl_crosshaircolor_r 255; cl_crosshaircolor_g 0; cl_crosshaircolor_b 0; \
                //            cl_crosshairalpha 255; cl_crosshaircolor 5; cl_crosshair_drawoutline 0; \
                //            cl_crosshairthickness 0.1; cl_crosshairdot 1; cl_crosshair_t 0; \
                //            cl_crosshairstyle 4; cl_crosshairsize 2");
                //        wasPressed = true;
                //    }
                //    // If just released
                //    else if (!isPressed && wasPressed) {
                //        ErScripts::CommandsSender(3, std::format(
                //            "cl_crosshairgap {}; cl_crosshair_outlinethickness {}; cl_crosshaircolor_r {}; \
                //            cl_crosshaircolor_g {}; cl_crosshaircolor_b {}; cl_crosshairalpha {}; \
                //            cl_crosshaircolor {}; cl_crosshair_drawoutline {}; cl_crosshairthickness {}; \
                //            cl_crosshairdot {}; cl_crosshair_t {}; cl_crosshairstyle {}; cl_crosshairsize {}",
                //            globals::config->gap,
                //            globals::config->outlineThickness,
                //            globals::config->red,
                //            globals::config->green,
                //            globals::config->blue,
                //            globals::config->alpha,
                //            globals::config->color,
                //            globals::config->drawOutline,
                //            globals::config->thickness,
                //            globals::config->dot,
                //            globals::config->tStyle,
                //            globals::config->style,
                //            globals::config->size));
                //        wasPressed = false;
                //    }
                //}

            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    }).detach();
}