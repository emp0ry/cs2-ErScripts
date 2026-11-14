#include "ErScripts.h"

void ErScripts::AutoAccept() {
    startWorker("AutoAccept", [this]() {
        const std::array<int, 3> targetColor = {54, 183, 82};

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->autoAcceptState) {
                if (foundMatch.exchange(false)) {

                    POINT mousePos{};
                    if (!GetCursorPos(&mousePos))
                        continue;

                    bool isInCS2 = GetWindowState();

                    ShowWindow(hwnd, SW_RESTORE);
                    SetForegroundWindow(hwnd);

                    bool colorState = false;
                    auto startTime = std::chrono::steady_clock::now();

                    while (
                        !globals::finish && Config::snapshot()->autoAcceptState &&
                        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime)
                                .count() <= Config::snapshot()->autoAcceptWaitingTime) {
                        const auto bounds = globals::getWindowBounds();
                        int buttonX = static_cast<int>(round(bounds.width / 2.f + bounds.x)),
                            buttonY = static_cast<int>(round(bounds.height / 2.215f + bounds.y));

                        if (GetWindowState()) {
                            const auto color = GetPixelColor(buttonX, buttonY);

                            if (color && isColorSimilar(targetColor, *color, 20)) {
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

                    if (!isInCS2 && !globals::finish) {
                        Keyboard(VK_MENU, true);
                        std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                        Keyboard(VK_TAB, true);
                        std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                        Keyboard(VK_TAB, false);
                        std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                        Keyboard(VK_MENU, false);
                    }

                    SetCursorPos(mousePos.x, mousePos.y);
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
