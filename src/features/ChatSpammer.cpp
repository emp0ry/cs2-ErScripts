#include "features/Features.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "game/GameWindow.h"
#include "game/GameConsole.h"
#include <format>

void Features::ChatSpammer() {
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
                    console.send(9, std::format("say {}", settings->chatSpammerText));
                }
            } else {
                state = statePrev = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
}
