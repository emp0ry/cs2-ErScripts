#include "ErScripts.h"

void ErScripts::ChatSpammer() {
	std::thread([this]() {
		bool state = false;
		bool statePrev = false;

		while (!globals::finish) {
			if (cfg->chatSpammerState) {
				bool bindState = GetAsyncKeyState(cfg->chatSpammerBind) & 0x8000;

				if (bindState && !statePrev) {
					state = !state;
				}

				statePrev = bindState;

				if (state || !cfg->chatSpammerBind) {
					CommandsSender(9, std::format("say {}", cfg->chatSpammerText));
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}).detach();
}