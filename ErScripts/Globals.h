#pragma once

#include "SteamTools.h"
#include "GSIPlayerState.h"
#include "CrosshairGeometry.h"
#include "GameVideo.h"
#include <windows.h>
#include <mutex>
#include <atomic>

namespace globals {
    extern int width, height, posX, posY;
    extern std::atomic_bool finish;
    extern bool menuState;

    extern std::atomic_bool isScope;
    void updatePlayerState(const nlohmann::json &payload);
    gsi::PlayerState getPlayerState();
    std::optional<crosshair::Frame> getSniperFrame(const crosshair::Settings &settings, int height);
    extern bool bombState;
    extern bool defusekitState;
    extern double bombTime;
    extern bool knifeState;
    extern bool pistolState;
    extern bool revolverState, cz75aState;
    extern bool roundStartState;
    extern bool isBombInWeapons;
    extern int localPlayerKills, localPlayerSlotNumber;
    extern bool localPlayerIsActivityPlaying;
    extern std::string steamid, nickname;

    extern bool previewMode;
    void setVideoSize(video::Size size);
    video::Size getVideoSize();
    std::optional<SteamTools::Config> getConfig();
    void setConfig(const SteamTools::Config &settings);
    void queueCrosshair(const SteamTools::Config &settings);
    bool hasPendingCrosshair();
    void flushCrosshair();

    extern int cs2_ping;

} // namespace globals
