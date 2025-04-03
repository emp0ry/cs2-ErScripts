#include "ErScripts.h"

void ErScripts::Crosshair() {
	std::thread([this]() {
		std::vector<int> blackColor = { 0, 0, 0 };
		bool oldRecoilCrosshairState = false;

		while (!globals::finish) {
			if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {

				// No longer needed because the print_changed_convars command is used for it
				/*if (globals::crosshairUpdaterState) {
					ErScripts::CommandsSender("host_writeconfig");
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					Logger::logInfo("Crosshair Updater");
					globals::crosshair = SteamTools::getCrosshairSettings("730");
					SteamTools::printCrosshairSettings(*globals::crosshair);
					globals::crosshairUpdaterState = false;
				}*/

				if (cfg->sniperCrosshairState && globals::sniperCrosshairState) {
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

				if (globals::crosshairUpdaterState) {
					ErScripts::CommandsSender(3, "print_changed_convars");
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}).detach();
}