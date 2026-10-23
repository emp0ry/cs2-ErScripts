#include "Globals.h"
#include "ErScripts.h"
#include "SniperDynamics.h"

namespace globals {
    int width = GetSystemMetrics(0), height = GetSystemMetrics(1), posX = 0, posY = 0;
    std::atomic_bool finish = false;
    bool menuState = true;

    std::atomic_bool isScope = false;
    namespace {
        std::mutex playerMutex;
        gsi::AmmoTracker playerTracker;
        crosshair::SniperDynamics sniperDynamics;
    } // namespace
    void updatePlayerState(const nlohmann::json &payload) {
        std::lock_guard lock(playerMutex);
        const auto now = gsi::Clock::now();
        playerTracker.update(payload, now);
        sniperDynamics.update(playerTracker.snapshot(now), now);
    }
    gsi::PlayerState getPlayerState() {
        std::lock_guard lock(playerMutex);
        return playerTracker.snapshot();
    }
    std::optional<crosshair::Frame> getSniperFrame(const crosshair::Settings &settings, int height) {
        std::lock_guard lock(playerMutex);
        const auto now = gsi::Clock::now();
        if (!playerTracker.snapshot(now).sniper())
            return std::nullopt;
        return sniperDynamics.frame(settings, height, now);
    }
    bool bombState = false;
    bool defusekitState = false;
    double bombTime = 0.0l;
    bool knifeState = false;
    bool pistolState = false;
    bool revolverState = false, cz75aState = false;
    bool roundStartState = false;
    bool isBombInWeapons = false;
    int localPlayerKills = 0, localPlayerSlotNumber = 0;
    bool localPlayerIsActivityPlaying = false;
    std::string steamid = "", nickname = "";

    bool previewMode = false;
    namespace {
        std::mutex configMutex;
        std::optional<SteamTools::Config> config = SteamTools::Config{};
        std::optional<SteamTools::Config> pendingCrosshair;
        unsigned revision = 0;
        std::optional<video::Size> videoSize;
    } // namespace
    void setVideoSize(video::Size size) {
        if (!size.valid())
            return;
        std::lock_guard lock(configMutex);
        videoSize = size;
    }
    video::Size getVideoSize() {
        std::lock_guard lock(configMutex);
        return previewMode ? video::Size{width, height} : videoSize.value_or(video::Size{width, height});
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

    int cs2_ping = 0;

} // namespace globals
