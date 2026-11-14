#include "features/Features.h"
#include "core/Globals.h"
#include <algorithm>

void Features::BombTimer() {
    startWorker("BombTimer", [this] {
        bool planted = false;
        std::uint64_t session = 0;
        std::chrono::steady_clock::time_point start;
        while (!globals::finish) {
            const auto game = globals::getGameState();
            if (game.bombPlanted) {
                const auto now = std::chrono::steady_clock::now();
                if (!planted || session != game.session) {
                    start = now;
                    planted = true;
                }
                globals::bombTime =
                    std::max(0.0, 40000.0 - std::chrono::duration<double, std::milli>(now - start).count());
            } else {
                planted = false;
                globals::bombTime = 0.0;
            }
            session = game.session;
            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
