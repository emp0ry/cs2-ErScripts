#include "ErScripts.h"

void ErScripts::ChatSpammer() {
    startWorker("ChatSpammer", [this]() {
        bool state = false;
        bool statePrev = false;

        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (settings->chatSpammerState) {
                bool bindState = GetAsyncKeyState(settings->chatSpammerBind) & 0x8000;

                if (bindState && !statePrev) {
                    state = !state;
                }

                statePrev = bindState;

                if (state || !settings->chatSpammerBind) {
                    CommandsSender(9, std::format("say {}", settings->chatSpammerText));
                }
            } else {
                state = statePrev = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
}
