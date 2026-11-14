#include "ErScripts.h"

void ErScripts::CS2Binds() {
    startWorker("CS2Binds", []() {
        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
                const auto crosshair = globals::getConfig().value_or(SteamTools::Config{});
                if (settings->longJumpBind) {
                    if (GetAsyncKeyState(settings->longJumpBind) & 0x8000) {
                        ErScripts::CommandsSender(7, "jump 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(7, "duck 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(7, "jump -999 1 0");
                        while ((GetAsyncKeyState(settings->longJumpBind) & 0x8000)) {
                            if (globals::finish || Config::snapshot()->longJumpBind != settings->longJumpBind ||
                                !GetWindowState() || !GetCursorState())
                                break;
                            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        }
                        ErScripts::CommandsSender(7, "duck -999 1 0");
                    }
                }

                if (settings->jumpThrowBind) {
                    if (GetAsyncKeyState(settings->jumpThrowBind) & 0x8000) {
                        ErScripts::CommandsSender(7, "jump 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(7, "attack -999 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(7, "jump -999 1 0");
                    }
                }

                if (settings->dropBombBind) {
                    if (globals::isBombInWeapons) {
                        if (GetAsyncKeyState(settings->dropBombBind) & 0x8000) {
                            ErScripts::CommandsSender(7, "slot5");
                            std::this_thread::sleep_for(std::chrono::milliseconds(64));
                            ErScripts::CommandsSender(7, "drop;lastinv");
                            std::this_thread::sleep_for(std::chrono::milliseconds(128));
                        }
                    }
                }

                if (settings->selfKickBind) {
                    if (GetAsyncKeyState(settings->selfKickBind) & 0x8000) {
                        ErScripts::CommandsSender(7, "status");
                        std::this_thread::sleep_for(std::chrono::milliseconds(256));
                        ErScripts::CommandsSender(
                            7, std::format("callvote kick {}", globals::localPlayerSlotNumber.load()));
                    }
                }

                static bool isBindPressed = false;
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
                        ErScripts::CommandsSender(7, std::format("yaw {} 1 0", yaw));
                        isBindPressed = true;
                    } else if (!isPressed && isBindPressed) {
                        isBindPressed = false;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }
    });
}
