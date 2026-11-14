#include "core/config.hpp"
#include "core/globals.hpp"
#include "core/logger.hpp"
#include "features/features.hpp"
#include "game/game_console.hpp"
#include "game/game_window.hpp"
#include "platform/input.hpp"

#include <array>
#include <cmath>
#include <cstdlib>

void Features::autoAccept() {
    startWorker("AutoAccept", [this]() {
        const std::array<int, 3> targetColor = {54, 183, 82};

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->autoAcceptState) {
                if (console_.takeMatch()) {

                    POINT mousePos{};
                    if (!GetCursorPos(&mousePos))
                        continue;

                    bool isInCS2 = game::hasFocus();

                    ShowWindow(game::window(), SW_RESTORE);
                    SetForegroundWindow(game::window());

                    bool colorState = false;
                    auto startTime = std::chrono::steady_clock::now();

                    while (
                        !globals::finish && Config::snapshot()->autoAcceptState &&
                        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime)
                                .count() <= Config::snapshot()->autoAcceptWaitingTime) {
                        const auto bounds = globals::getWindowBounds();
                        int buttonX = static_cast<int>(round(bounds.width / 2.f + bounds.x)),
                            buttonY = static_cast<int>(round(bounds.height / 2.215f + bounds.y));

                        if (game::hasFocus()) {
                            const auto color = getPixelColor(buttonX, buttonY);

                            if (color && isColorSimilar(targetColor, *color, 20)) {
                                colorState = true;
                                Logger::logInfo("AutoAccept button clicking");

                                if (!SetCursorPos(buttonX, buttonY))
                                    break;
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));

                                input::KeyPress click(VK_LBUTTON);
                                std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                            } else if (colorState) {
                                break;
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }

                    if (!isInCS2 && !globals::finish) {
                        input::KeyPress alt(VK_MENU);
                        if (alt.active()) {
                            std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                            {
                                input::KeyPress tab(VK_TAB);
                                if (tab.active())
                                    std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 32) + 16));
                        }
                    }

                    SetCursorPos(mousePos.x, mousePos.y);
                }
            } else {
                console_.takeMatch();
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
