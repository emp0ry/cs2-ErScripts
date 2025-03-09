#include "CS2Functions.h"

void CS2Functions::AutoAccept() {
	std::thread([this]() {
		const std::vector<int> targetColor = { 54, 183, 82 };

		while (!globals::finish) {
			if (!cfg->autoAcceptState)
				continue;

			if (foundMatch) {
				foundMatch = false;

				POINT mousePos;
				GetCursorPos(&mousePos);

				bool isInCS2 = GetWindowState();

				// Maximize cs2
				ShowWindow(hwnd, SW_RESTORE);
				SetForegroundWindow(hwnd);

				bool colorState = false;
				auto startTime = std::chrono::steady_clock::now();

				while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count() <= cfg->autoAcceptWaitingTime) {
					int buttonX = static_cast<int>(round(globals::width / 2.f + globals::posX)), buttonY = static_cast<int>(round(globals::height / 2.215f + globals::posY));

					if (GetWindowState()) {
						std::vector color = GetPixelColor(buttonX, buttonY);

						if (isColorSimilar(targetColor, color, 20)) {
							colorState = true;
							Logger::logInfo("AutoAccept button clicking");

							// Move cursor to correct position
							SetCursorPos(buttonX, buttonY);
							std::this_thread::sleep_for(std::chrono::milliseconds(1));

							// Click accept button
							mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
							std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
							mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
						}
						else if (colorState) {
							break;
						}
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}

				if (!isInCS2) {
					Keyboard(56, true, false);
					std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
					Keyboard(15, true, false);
					std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
					Keyboard(15, false, false);
					std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
					Keyboard(56, false, false);
				}

				SetCursorPos(mousePos.x, mousePos.y);
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}).detach();
}