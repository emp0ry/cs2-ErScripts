#include "ErScripts.h"

void ErScripts::CS2Binds() {
    std::thread([]() {
        while (!globals::finish) {
            if (ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
                const auto settings = globals::getConfig().value_or(SteamTools::Config{});
                if (cfg->longJumpBind) {
                    if (GetAsyncKeyState(cfg->longJumpBind) & 0x8000) {
                        ErScripts::CommandsSender(7, "jump 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(7, "duck 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(7, "jump -999 1 0");
                        while ((GetAsyncKeyState(cfg->longJumpBind) & 0x8000)) {
                            if (globals::finish)
                                break;
                            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        }
                        ErScripts::CommandsSender(7, "duck -999 1 0");
                    }
                }

                if (cfg->jumpThrowBind) {
                    if (GetAsyncKeyState(cfg->jumpThrowBind) & 0x8000) {
                        ErScripts::CommandsSender(7, "jump 1 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(7, "attack -999 1 0");
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 5) + 1));
                        ErScripts::CommandsSender(7, "jump -999 1 0");
                    }
                }

                if (cfg->dropBombBind) {
                    if (globals::isBombInWeapons) {
                        if (GetAsyncKeyState(cfg->dropBombBind) & 0x8000) {
                            ErScripts::CommandsSender(7, "slot5");
                            std::this_thread::sleep_for(std::chrono::milliseconds(64));
                            ErScripts::CommandsSender(7, "drop;lastinv");
                            std::this_thread::sleep_for(std::chrono::milliseconds(128));
                        }
                    }
                }

                if (cfg->selfKickBind) {
                    if (GetAsyncKeyState(cfg->selfKickBind) & 0x8000) {
                        ErScripts::CommandsSender(7, "status");
                        std::this_thread::sleep_for(std::chrono::milliseconds(256));
                        ErScripts::CommandsSender(7, std::format("callvote kick {}", globals::localPlayerSlotNumber));
                    }
                }

                static bool isBindPressed = false;
                if (cfg->angleBindState && settings.sensitivity != 0 && settings.zoomSensitivity != 0 &&
                    settings.yaw != 0) {
                    bool isPressed = GetAsyncKeyState(cfg->angleBindBind) & 0x8000;
                    if (isPressed && !isBindPressed) {
                        float yaw = 0.0f;
                        if (globals::isScope) {
                            yaw = roundf((cfg->angleBindDegree /
                                          (settings.sensitivity * settings.zoomSensitivity * 0.4444444444f) /
                                          settings.yaw) *
                                         10000.0f) /
                                  10000.0f;
                        } else {
                            yaw = roundf((cfg->angleBindDegree / settings.sensitivity / settings.yaw) * 10000.0f) /
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
    }).detach();
}
