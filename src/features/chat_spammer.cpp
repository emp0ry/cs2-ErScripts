#include "core/config.hpp"
#include "core/globals.hpp"
#include "features/features.hpp"
#include "game/game_console.hpp"
#include "game/game_window.hpp"

#include <format>

void Features::chatSpammer() {
    startWorker("ChatSpammer", [this]() {
        bool state = false;
        bool statePrev = false;

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->chatSpammerState && game::hasFocus() && game::cursorHidden()) {
                bool bindState = GetAsyncKeyState(settings->chatSpammerBind) & 0x8000;

                if (bindState && !statePrev) {
                    state = !state;
                }

                statePrev = bindState;

                if (state || !settings->chatSpammerBind) {
                    console_.send(9, std::format("say {}", settings->chatSpammerText));
                }
            } else {
                state = statePrev = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
}
