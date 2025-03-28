#include "ErScripts.h"
#include <random>

inline int GenerateUniqueRandomDelay(std::vector<int>& recentDelays, int minDelay, int maxDelay) {
    static std::mt19937 rng(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(minDelay, maxDelay);

    int delay;
    do {
        delay = dist(rng);
    } while (std::find(recentDelays.begin(), recentDelays.end(), delay) != recentDelays.end());

    if (recentDelays.size() >= 10) {
        recentDelays.erase(recentDelays.begin());
    }
    recentDelays.push_back(delay);

    return delay;
}

inline void PressAndRelease(char key, std::vector<int>& recentDelays) {
    int minDelay = 5;       // Min delay in ms
    int maxDelay = 30;      // Max delay in ms
    int baseTimeout = 115;  // Base timeout (115)

    int delay = GenerateUniqueRandomDelay(recentDelays, minDelay, maxDelay);

    int totalTimeout = baseTimeout - delay;

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    ErScripts::Keyboard(key, true, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(totalTimeout));
    ErScripts::Keyboard(key, false, false);
}

void ErScripts::AutoStop() {
    std::thread([this]() {
        std::vector<int> recentDelays;
        bool aWasPressed = false;
        bool dWasPressed = false;

        bool state = false;
        bool statePrev = false;

        while (!globals::finish) {
            if (cfg->autoStopState) {
                if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
                    bool bindState = GetAsyncKeyState(cfg->autoStopBind) & 0x8000;

                    if (cfg->autoStopToggleState) {
                        if (bindState && !statePrev) {
                            state = !state;
                        }
                    }
                    else {
                        state = bindState;
                    }
                    statePrev = bindState;

                    if (state) {
                        bool aPressed = GetAsyncKeyState('A') & 0x8000;
                        bool dPressed = GetAsyncKeyState('D') & 0x8000;

                        // Detect release of A while not pressing D
                        if (aWasPressed && !aPressed && !dPressed) {
                            PressAndRelease('D', recentDelays);
                        }
                        // Detect release of D while not pressing A
                        else if (dWasPressed && !dPressed && !aPressed) {
                            PressAndRelease('A', recentDelays);
                        }

                        aWasPressed = aPressed;
                        dWasPressed = dPressed;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }).detach();
}