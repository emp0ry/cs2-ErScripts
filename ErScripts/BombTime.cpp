#include "ErScripts.h"

void ErScripts::BombTimer() {
    startWorker("BombTimer", [] {
        bool planted = false;
        std::chrono::steady_clock::time_point start;
        while (!globals::finish) {
            if (globals::bombState) {
                const auto now = std::chrono::steady_clock::now();
                if (!planted) {
                    start = now;
                    planted = true;
                }
                globals::bombTime =
                    std::max(0.0, 40000.0 - std::chrono::duration<double, std::milli>(now - start).count());
            } else {
                planted = false;
                globals::bombTime = 0.0;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
