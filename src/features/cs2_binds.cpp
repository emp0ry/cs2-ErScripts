#include "core/config.hpp"
#include "core/globals.hpp"
#include "features/features.hpp"
#include "game/game_console.hpp"
#include "game/game_window.hpp"

#include <cstdlib>
#include <format>

void Features::cS2Binds() {
    startWorker("CS2Binds", [this]() {
        bool jumpPending = false, duckPending = false, isBindPressed = false;
        const auto release = [&] {
            if (!jumpPending && !duckPending)
                return true;
            const char* command = jumpPending && duckPending ? "jump -999 1 0; duck -999 1 0"
                                  : jumpPending              ? "jump -999 1 0"
                                                             : "duck -999 1 0";
            if (!console_.send(7, command))
                return false;
            jumpPending = duckPending = false;
            return true;
        };
        while (!globals::finish) {
            if (!release()) {
                isBindPressed = false;
                std::this_thread::sleep_for(std::chrono::microseconds(15625));
                continue;
            }
            const auto settings = Config::snapshot();
            const auto game = globals::getGameState();
            if (game::hasFocus() && game::cursorHidden()) {
                const auto crosshair = globals::getConfig().value_or(SteamTools::Config{});
                if (settings->longJumpBind) {
                    if ((GetAsyncKeyState(settings->longJumpBind) & 0x8000) && console_.send(7, "jump 1 1 0")) {
                        jumpPending = true;
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        duckPending = console_.send(7, "duck 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        jumpPending = !console_.send(7, "jump -999 1 0");
                        while ((GetAsyncKeyState(settings->longJumpBind) & 0x8000)) {
                            if (globals::finish || Config::snapshot()->longJumpBind != settings->longJumpBind ||
                                !game::hasFocus() || !game::cursorHidden())
                                break;
                            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        }
                        release();
                    }
                }

                if (!jumpPending && !duckPending && settings->jumpThrowBind) {
                    if ((GetAsyncKeyState(settings->jumpThrowBind) & 0x8000) && console_.send(7, "jump 1 1 0")) {
                        jumpPending = true;
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        console_.send(7, "attack -999 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        release();
                    }
                }

                if (settings->dropBombBind) {
                    if (game.bombInWeapons) {
                        if (GetAsyncKeyState(settings->dropBombBind) & 0x8000) {
                            console_.send(7, "slot5");
                            std::this_thread::sleep_for(std::chrono::milliseconds(64));
                            console_.send(7, "drop;lastinv");
                            std::this_thread::sleep_for(std::chrono::milliseconds(128));
                        }
                    }
                }

                if (settings->selfKickBind) {
                    if (GetAsyncKeyState(settings->selfKickBind) & 0x8000) {
                        globals::localPlayerSlotNumber = -1;
                        if (console_.send(7, "status")) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(256));
                            const int slot = globals::localPlayerSlotNumber.load();
                            if (slot >= 0 && !globals::finish &&
                                Config::snapshot()->selfKickBind == settings->selfKickBind &&
                                globals::getGameState().session == game.session)
                                console_.send(7, std::format("callvote kick {}", slot));
                        }
                    }
                }

                if (settings->angleBindState && crosshair.sensitivity != 0 && crosshair.zoomSensitivity != 0 &&
                    crosshair.yaw != 0) {
                    bool isPressed = GetAsyncKeyState(settings->angleBindBind) & 0x8000;
                    if (isPressed && !isBindPressed) {
                        float yaw = 0.0f;
                        if (globals::isScope) {
                            yaw = roundf((settings->angleBindDegree /
                                          (crosshair.sensitivity * crosshair.zoomSensitivity * 0.4444444444f) /
                                          crosshair.yaw) *
                                         10000.0f) /
                                  10000.0f;
                        } else {
                            yaw =
                                roundf((settings->angleBindDegree / crosshair.sensitivity / crosshair.yaw) * 10000.0f) /
                                10000.0f;
                        }
                        if (std::isfinite(yaw))
                            isBindPressed = console_.send(7, std::format("yaw {} 1 0", yaw));
                    } else if (!isPressed && isBindPressed) {
                        isBindPressed = false;
                    }
                } else {
                    isBindPressed = false;
                }
            } else {
                isBindPressed = false;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
        release();
    });
}
