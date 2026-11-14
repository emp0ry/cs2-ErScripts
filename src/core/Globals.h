#pragma once

class GameConsole;

#include "game/SteamTools.h"
#include "game/GSIState.h"
#include "graphics/CrosshairGeometry.h"
#include "game/GameVideo.h"
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
    bool updateGameState(const nlohmann::json &payload);
    gsi::PlayerState getPlayerState();
    std::optional<crosshair::Frame> getSniperFrame(const crosshair::Settings &settings, int height);
    gsi::GameState getGameState();
    extern std::atomic<double> bombTime;
    extern std::atomic_int localPlayerSlotNumber;
    extern std::string nickname;

    extern bool previewMode;
    void setVideoSize(video::Size size);
    video::Size getVideoSize();
    std::optional<SteamTools::Config> getConfig();
    void setConfig(const SteamTools::Config &settings);
    void queueCrosshair(const SteamTools::Config &settings);
    bool hasPendingCrosshair();
    void flushCrosshair(GameConsole &console);

    extern std::atomic<int> cs2_ping;

} // namespace globals
