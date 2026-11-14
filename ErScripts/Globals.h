#pragma once

#include "SteamTools.h"
#include "GSIPlayerState.h"
#include "CrosshairGeometry.h"
#include "GameVideo.h"
#include <windows.h>
#include <mutex>
#include <atomic>

namespace globals {
    // Render-thread coordinates. Workers use getWindowBounds().
    extern int width, height, posX, posY;
    struct WindowBounds {
        int width, height, x, y;
        bool operator==(const WindowBounds &) const = default;
    };
    WindowBounds getWindowBounds();
    void setWindowBounds(WindowBounds bounds);
    extern std::atomic_bool finish;
    extern std::atomic<bool> menuState;

    extern std::atomic_bool isScope;
    bool updatePlayerState(const nlohmann::json &payload);
    gsi::PlayerState getPlayerState();
    std::optional<crosshair::Frame> getSniperFrame(const crosshair::Settings &settings, int height);
    extern std::atomic<bool> bombState;
    extern std::atomic<bool> defusekitState;
    extern std::atomic<double> bombTime;
    extern std::atomic<bool> knifeState;
    extern std::atomic<bool> pistolState;
    extern std::atomic_bool revolverState, cz75aState;
    extern std::atomic<bool> roundStartState;
    extern std::atomic<bool> isBombInWeapons;
    extern std::atomic_int localPlayerKills, localPlayerSlotNumber;
    extern std::atomic<bool> localPlayerIsActivityPlaying;
    extern std::string steamid, nickname;

    extern bool previewMode;
    void setVideoSize(video::Size size);
    video::Size getVideoSize();
    std::optional<SteamTools::Config> getConfig();
    void setConfig(const SteamTools::Config &settings);
    void queueCrosshair(const SteamTools::Config &settings);
    bool hasPendingCrosshair();
    void flushCrosshair();

    extern std::atomic<int> cs2_ping;

} // namespace globals
