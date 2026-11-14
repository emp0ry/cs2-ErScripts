#include "Globals.h"
#include "ErScripts.h"
#include "SniperDynamics.h"

namespace globals {
    int width = GetSystemMetrics(0), height = GetSystemMetrics(1), posX = 0, posY = 0;
    std::atomic_bool finish = false;
    std::atomic<bool> menuState = true;

    std::atomic_bool isScope = false;
    namespace {
        std::mutex playerMutex;
        gsi::AmmoTracker playerTracker;
        crosshair::SniperDynamics sniperDynamics;
    } // namespace
    bool updatePlayerState(const nlohmann::json &payload) {
        std::lock_guard lock(playerMutex);
        const auto now = gsi::Clock::now();
        if (!playerTracker.update(payload, now))
            return false;
        sniperDynamics.update(playerTracker.snapshot(now), now);
        return true;
    }
    gsi::PlayerState getPlayerState() {
        std::lock_guard lock(playerMutex);
        return playerTracker.snapshot();
    }
    std::optional<crosshair::Frame> getSniperFrame(const crosshair::Settings &settings, int viewportHeight) {
        std::lock_guard lock(playerMutex);
        const auto now = gsi::Clock::now();
        if (!playerTracker.snapshot(now).sniper())
            return std::nullopt;
        return sniperDynamics.frame(settings, viewportHeight, now);
    }
    std::atomic<bool> bombState = false;
    std::atomic<bool> defusekitState = false;
    std::atomic<double> bombTime = 0.0l;
    std::atomic<bool> knifeState = false;
    std::atomic<bool> pistolState = false;
    std::atomic_bool revolverState = false, cz75aState = false;
    std::atomic<bool> roundStartState = false;
    std::atomic<bool> isBombInWeapons = false;
    std::atomic_int localPlayerKills = 0, localPlayerSlotNumber = 0;
    std::atomic<bool> localPlayerIsActivityPlaying = false;
    std::string steamid = "", nickname = "";

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
    void flushCrosshair() {
        std::optional<SteamTools::Config> draft;
        unsigned version;
        {
            std::lock_guard lock(configMutex);
            draft = pendingCrosshair;
            version = revision;
        }
        if (draft && ErScripts::CommandsSender(10, crosshair::commands(*draft))) {
            std::lock_guard lock(configMutex);
            if (version == revision) {
                config = draft;
                pendingCrosshair.reset();
            }
        }
    }

    std::atomic<int> cs2_ping = 0;

} // namespace globals
