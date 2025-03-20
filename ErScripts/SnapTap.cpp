#include "ErScripts.h"
#include <random>

inline int GenerateUniqueRandomDelay(std::vector<int>& recentDelays, int minDelay, int maxDelay) {
    static std::mt19937 rng(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(minDelay, maxDelay);

    int delay;
    do {
        delay = dist(rng);
    } while (std::find(recentDelays.begin(), recentDelays.end(), delay) != recentDelays.end());

    if (recentDelays.size() >= 5) {
        recentDelays.erase(recentDelays.begin());
    }
    recentDelays.push_back(delay);

    return delay;
}

// Not working yet
void ErScripts::SnapTap() {
    std::thread([this]() {
        std::vector<int> recentDelays;
        const int minDelay = 1;
        const int maxDelay = 50;

        bool aWasPressed = false;
        bool dWasPressed = false;

        while (!globals::finish) {
            if (cfg->snapTapState) {
                if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
                    bool aPressed = GetAsyncKeyState('A') & 0x8000;
                    bool dPressed = GetAsyncKeyState('D') & 0x8000;

                    // Detect release of 'A' while not pressing 'D'
                    if (aWasPressed && !aPressed && !dPressed) {
                        int delay = GenerateUniqueRandomDelay(recentDelays, minDelay, maxDelay);
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        Keyboard('D', true, false);
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay * 2));
                        Keyboard('D', false, false);
                    }
                    // Detect release of 'D' while not pressing 'A'
                    else if (dWasPressed && !dPressed && !aPressed) {
                        int delay = GenerateUniqueRandomDelay(recentDelays, minDelay, maxDelay);
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        Keyboard('A', true, false);
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay * 2));
                        Keyboard('A', false, false);
                    }

                    aWasPressed = aPressed;
                    dWasPressed = dPressed;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        }).detach();
}