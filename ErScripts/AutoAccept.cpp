#include "ErScripts.h"

void ErScripts::AutoAccept() {
    std::thread([this]() {
        const std::vector<int> targetColor = {54, 183, 82};

        while (!globals::finish) {
            if (cfg->autoAcceptState) {
                if (foundMatch) {
                    foundMatch = false;

                    POINT mousePos;
                    GetCursorPos(&mousePos);

                    bool isInCS2 = GetWindowState();

                    ShowWindow(hwnd, SW_RESTORE);
                    SetForegroundWindow(hwnd);

                    bool colorState = false;
                    auto startTime = std::chrono::steady_clock::now();

                    while (
                        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime)
                            .count() <= cfg->autoAcceptWaitingTime) {
                        int buttonX = static_cast<int>(round(globals::width / 2.f + globals::posX)),
                            buttonY = static_cast<int>(round(globals::height / 2.215f + globals::posY));

                        if (GetWindowState()) {
                            std::vector color = GetPixelColor(buttonX, buttonY);

                            if (isColorSimilar(targetColor, color, 20)) {
                                colorState = true;
                                Logger::logInfo("AutoAccept button clicking");

                                SetCursorPos(buttonX, buttonY);
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));

                                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                                std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                            } else if (colorState) {
                                break;
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }

                    if (!isInCS2) {
                        Keyboard(56, true);
                        std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                        Keyboard(15, true);
                        std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                        Keyboard(15, false);
                        std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                        Keyboard(56, false);
                    }

                    SetCursorPos(mousePos.x, mousePos.y);
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    }).detach();
}
