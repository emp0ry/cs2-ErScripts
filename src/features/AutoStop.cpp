#include "features/Features.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "game/GameWindow.h"
#include "platform/Input.h"
#include <cstdlib>
#include <algorithm>
#include "features/AutoStop.h"
#include <stdexcept>

int autostop::randomDelay(std::vector<int> &recent, int minimum, int maximum, std::mt19937 &rng) {
    if (minimum > maximum)
        throw std::invalid_argument("Invalid delay range");
    std::erase_if(recent, [=](int value) { return value < minimum || value > maximum; });
    const auto range = static_cast<std::uint64_t>(static_cast<std::int64_t>(maximum) - minimum) + 1;
    while (recent.size() >= std::min<std::uint64_t>(11, range))
        recent.erase(recent.begin());
    auto delay = std::uniform_int_distribution<int>(minimum, maximum)(rng);
    // At most ten values are excluded. One draw and a bounded scan always find an available delay.
    for (size_t attempt = 0; attempt <= recent.size(); ++attempt) {
        if (std::find(recent.begin(), recent.end(), delay) == recent.end()) {
            if (recent.size() == 10)
                recent.erase(recent.begin());
            recent.push_back(delay);
            return delay;
        }
        delay = delay == maximum ? minimum : delay + 1;
    }
    throw std::logic_error("Delay history has no available value");
}

namespace {
    void pressAndRelease(char key, std::vector<int> &recentDelays) {
        int minDelay = 5;
        int maxDelay = 30;
        int baseTimeout = 115;

        thread_local std::mt19937 rng(std::random_device{}());
        int delay = autostop::randomDelay(recentDelays, minDelay, maxDelay, rng);

        int totalTimeout = baseTimeout - delay;

        const auto active = [] {
            const auto settings = Config::snapshot();
            return !globals::finish && settings->autoStopState && game::hasFocus() && game::cursorHidden() &&
                   (settings->autoStopToggleState || (GetAsyncKeyState(settings->autoStopBind) & 0x8000));
        };
        const auto wait = [&](int milliseconds) {
            const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
            while (active()) {
                if (std::chrono::steady_clock::now() >= until)
                    return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return false;
        };
        if (!wait(delay))
            return;
        input::KeyPress press(key);
        if (press.active())
            wait(totalTimeout);
    }

} // namespace

void Features::AutoStop() {
    startWorker("AutoStop", [this]() {
        std::vector<int> recentDelays;
        bool aWasPressed = false;
        bool dWasPressed = false;

        bool state = false;
        bool statePrev = false;

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->autoStopState) {
                if (game::hasFocus() && game::cursorHidden()) {
                    bool bindState = GetAsyncKeyState(settings->autoStopBind) & 0x8000;

                    if (settings->autoStopToggleState) {
                        if (bindState && !statePrev) {
                            state = !state;
                        }
                    } else {
                        state = bindState;
                    }
                    statePrev = bindState;

                    if (state) {
                        bool aPressed = GetAsyncKeyState('A') & 0x8000;
                        bool dPressed = GetAsyncKeyState('D') & 0x8000;

                        if (aWasPressed && !aPressed && !dPressed) {
                            pressAndRelease('D', recentDelays);
                        }

                        else if (dWasPressed && !dPressed && !aPressed) {
                            pressAndRelease('A', recentDelays);
                        }

                        aWasPressed = aPressed;
                        dWasPressed = dPressed;
                    }
                }
            }
            if (!settings->autoStopState || !state || !game::hasFocus() || !game::cursorHidden()) {
                aWasPressed = dWasPressed = false;
                if (!settings->autoStopState || !game::hasFocus() || !game::cursorHidden())
                    state = statePrev = false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
