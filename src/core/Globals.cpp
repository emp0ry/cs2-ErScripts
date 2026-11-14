#include "core/Globals.h"
#include "game/GameConsole.h"
#include "graphics/SniperDynamics.h"

namespace globals {
    int width = GetSystemMetrics(0), height = GetSystemMetrics(1), posX = 0, posY = 0;
    std::atomic_bool finish = false;
    std::atomic<bool> menuState = true;

    std::atomic_bool isScope = false;
    namespace {
        std::mutex stateMutex;
        gsi::Tracker gameTracker;
        crosshair::SniperDynamics sniperDynamics;
    } // namespace
    bool updateGameState(const nlohmann::json &payload) {
        std::lock_guard lock(stateMutex);
        const auto now = gsi::Clock::now();
        const auto previousSession = gameTracker.gameSnapshot(now).session;
        if (!gameTracker.update(payload, now))
            return false;
        if (gameTracker.gameSnapshot(now).session != previousSession)
            localPlayerSlotNumber = -1;
        sniperDynamics.update(gameTracker.snapshot(now), now);
        return true;
    }
    gsi::PlayerState getPlayerState() {
        std::lock_guard lock(stateMutex);
        return gameTracker.snapshot();
    }
    std::optional<crosshair::Frame> getSniperFrame(const crosshair::Settings &settings, int viewportHeight) {
        std::lock_guard lock(stateMutex);
        const auto now = gsi::Clock::now();
        if (!gameTracker.snapshot(now).sniper())
            return std::nullopt;
        return sniperDynamics.frame(settings, viewportHeight, now);
    }
    gsi::GameState getGameState() {
        std::lock_guard lock(stateMutex);
        return gameTracker.gameSnapshot();
    }
    std::atomic<double> bombTime = 0.0;
    std::atomic_int localPlayerSlotNumber = -1;
    std::string nickname;

    bool previewMode = false;
    namespace {
        std::mutex configMutex;
        WindowBounds windowBounds{width, height, posX, posY};
        std::optional<SteamTools::Config> config = SteamTools::Config{};
        std::optional<SteamTools::Config> pendingCrosshair;
        unsigned revision = 0;
        std::optional<video::Size> videoSize;
    } // namespace
    WindowBounds getWindowBounds() {
        std::lock_guard lock(configMutex);
        return windowBounds;
    }
    void setWindowBounds(WindowBounds bounds) {
        std::lock_guard lock(configMutex);
        windowBounds = bounds;
    }
    void setVideoSize(video::Size size) {
        if (!size.valid())
            return;
        std::lock_guard lock(configMutex);
        videoSize = size;
    }
    video::Size getVideoSize() {
        std::lock_guard lock(configMutex);
        const video::Size size{windowBounds.width, windowBounds.height};
        return previewMode ? size : videoSize.value_or(size);
    }
    std::optional<SteamTools::Config> getConfig() {
        std::lock_guard lock(configMutex);
        return config;
    }
    void setConfig(const SteamTools::Config &settings) {
        std::lock_guard lock(configMutex);
        config = settings;
    }
    void queueCrosshair(const SteamTools::Config &settings) {
        std::lock_guard lock(configMutex);
        pendingCrosshair = settings;
        ++revision;
    }
    bool hasPendingCrosshair() {
        std::lock_guard lock(configMutex);
        return pendingCrosshair.has_value();
    }
    void flushCrosshair(GameConsole &console) {
        std::optional<SteamTools::Config> draft;
        unsigned version;
        {
            std::lock_guard lock(configMutex);
            draft = pendingCrosshair;
            version = revision;
        }
        if (draft && console.send(10, crosshair::commands(*draft))) {
            std::lock_guard lock(configMutex);
            if (version == revision) {
                config = draft;
                pendingCrosshair.reset();
            }
        }
    }

    std::atomic<int> cs2_ping = 0;

} // namespace globals
