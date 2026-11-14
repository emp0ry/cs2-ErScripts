#pragma once

#include "core/Logger.h"
#include "core/GradientManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <memory>
#include <string>
#include <cmath>
#include <limits>
#include <type_traits>

class Config {
  public:
    static constexpr const char *defaultChatMessage = "Customize CS2 with ErScripts: erscripts.emp0ry.com";

    // The render thread edits cfg. Workers only read published immutable snapshots.
    static std::shared_ptr<const Config> snapshot();
    void publish() const;
    bool operator==(const Config &) const = default;

    void load(const std::string &filename);
    bool save(const std::string &filename) const;

    bool triggerState{false};
    int triggerBind{0};
    int triggerDisplacement[2]{1, 1};
    int triggerThreshold{20};
    int triggerDelayMs{0};

    bool autoAcceptState{false};
    int autoAcceptWaitingTime{5};

    bool bombTimerState{false};
    float bombTimerScale{1.0f};
    bool bombTimerGradientState{false};
    float bombTimerTransparency{1.0f};
    float bombTimerPos[2]{0, 0};

    bool sniperCrosshairState{false};

    bool rgbCrosshairState{false};

    bool recoilCrosshairState{false};

    bool keystrokesState{false};
    float keystrokesScale{1.5f};
    bool keystrokesGradientState{true};
    float keystrokesAnimationSpeed{15.0f};
    RGBColor keystrokesPressedColor{15, 15, 15};
    RGBColor keystrokesReleasedColor{20, 20, 20};
    float keystrokesTransparency{0.6f};
    float keystrokesPos[2]{0, 0};

    bool knifeSwitchState{false};

    bool autoPistolState{false};

    bool antiAfkState{false};

    int longJumpBind{0};

    int jumpThrowBind{0};

    int dropBombBind{0};

    int selfKickBind{0};

    bool killSayState{false};
    std::string killSayText{"1"};

    bool killSoundState{false};
    int killSoundVolume{100};
    std::string killSoundFileName{""};

    bool roundStartAlertState{false};
    int roundStartAlertVolume{100};
    std::string roundStartAlertFileName{""};

    bool autoStopState{false};
    int autoStopBind{0};
    bool autoStopToggleState{false};

    bool chatSpammerState{false};
    int chatSpammerBind{0};
    std::string chatSpammerText{defaultChatMessage};

    bool angleBindState{false};
    int angleBindBind{0};
    float angleBindDegree{180.0f};

    bool watermarkState{true};
    bool watermarkGradientState{true};
    float watermarkTransparency{1.0f};
    int watermarkPingUpdateRate{3};

    bool fpsLimiterState{true};
    int fpsLimiter{120};

    bool captureBypassState{false};

    GradientConfig gradient;

    bool vsyncState{false};

    int erScriptsMenuBind{VK_INSERT};
    int erScriptsExitBind{VK_END};

  private:
    template <typename T> bool read(const nlohmann::json &src, T &dest) {
        try {
            if (src.is_null())
                return false;
            if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
                if (!src.is_number_integer() || src < std::numeric_limits<T>::lowest() ||
                    src > std::numeric_limits<T>::max())
                    throw std::runtime_error("Expected an integer within range");
            }
            const auto value = src.get<T>();
            if constexpr (std::is_floating_point_v<T>)
                if (!std::isfinite(value))
                    throw std::runtime_error("Expected a finite number");
            dest = value;
            return true;
        } catch (const std::exception &e) {
            Logger::logWarning(std::format("Config Error Code: {}", e.what()));
            return false;
        }
    }

    void readArray(const nlohmann::json &src, float (&dest)[2]);
    void readArray(const nlohmann::json &src, int (&dest)[2]);
    void readRGB(const nlohmann::json &src, RGBColor &dest);
};

inline std::unique_ptr<Config> cfg = std::make_unique<Config>();
