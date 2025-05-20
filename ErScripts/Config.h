#pragma once

#include "Logger.h"
#include "GradientManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <memory>
#include <string>

class Config {
public:
    void load(const std::string& filename);
    void save(const std::string& filename) const;

    /* Pixel Trigger */
    bool triggerState{ false };
    int triggerBind{ 0 };
    int triggerDisplacement[2]{ 1, 1 };
    int triggerThreshold{ 20 };
    int triggerDelayMs{ 0 };

    /* Auto Accept */
    bool autoAcceptState{ false };
    int autoAcceptWaitingTime{ 5 };

    /* Bomb Timer */
    bool bombTimerState{ false };
    float bombTimerScale{ 1.0f };
    bool bombTimerGradientState{ false };
    float bombTimerTransparency{ 1.0f };
    float bombTimerPos[2]{ 0, 0 };

    /* Sniper Crosshair */
    bool sniperCrosshairState{ false };

    /* RGB Crosshair */
    bool rgbCrosshairState{ false };

    /* Recoil Crosshair */
    bool recoilCrosshairState{ false };

    /* Keystrokes */
    bool keystrokesState{ false };
    float keystrokesScale{ 1.5f };
    bool keystrokesGradientState{ true };
    float keystrokesAnimationSpeed{ 15.0f };
    RGBColor keystrokesPressedColor{ 15, 15, 15 };
    RGBColor keystrokesReleasedColor{ 20, 20, 20 };
    float keystrokesTransparency{ 0.6f };
    float keystrokesPos[2]{ 0, 0 };

    /* Keystrokes */
    bool knifeSwitchState{ false };

    /* Auto pistol */
    bool autoPistolState{ false };

    /* Anti Afk */
    bool antiAfkState{ false };

    /* Long Jump */
    int longJumpBind{ 0 };

    /* Jump Throw */
    int jumpThrowBind{ 0 };

    /* Drop Bomb */
    int dropBombBind{ 0 };

    /* Self Kick */
    int selfKickBind{ 0 };

    /* Kill Say */
    bool killSayState{ false };
    std::string killSayText{ "1" };

    /* Kill Sound */
    bool killSoundState{ false };
    int killSoundVolume{ 100 };
    std::string killSoundFileName{ "" };

    /* Round Start Alert */
    bool roundStartAlertState{ false };
    int roundStartAlertVolume{ 100 };
    std::string roundStartAlertFileName{ "" };

    /* Auto Stop */
    bool autoStopState{ false };
    int autoStopBind{ 0 };
    bool autoStopToggleState{ false };

    /* Chat Spammer */
    bool chatSpammerState{ false };
    int chatSpammerBind{ 0 };
    std::string chatSpammerText{ "Enhance Your Skills with emp0ry.github.io/cs2-ErScripts/" };

    /* Angle Bind */
    bool angleBindState{ false };
    int angleBindBind{ 0 };
    float angleBindDegree{ 180.0f };

    /* Watermark */
    bool watermarkState{ true };
    bool watermarkGradientState{ true };
    float watermarkTransparency{ 1.0f };
    int watermarkPingUpdateRate{ 3 };

    /* FPS Limiter */
    bool fpsLimiterState{ true };
    int fpsLimiter{ 350 };

    /* Capture Bypass */
    bool captureBypassState{ false };

    /* Gradient Manager */
    GradientConfig gradient;

    /* Vsync */
    bool vsyncState{ false };

    /* ErScripts Binds */
    int erScriptsMenuBind{ VK_INSERT };
    int erScriptsExitBind{ VK_END };

private:
    template <typename T>
    void read(const nlohmann::json& src, T& dest) {
        try {
            if (!src.is_null()) {
                dest = src.get<T>();
            }
        }
        catch (const std::exception& e) {
            Logger::logWarning(std::format("Config Error Code: {}", e.what()));
        }
    }

    void readArray(const nlohmann::json& src, float(&dest)[2]);
    void readArray(const nlohmann::json& src, int(&dest)[2]);
    void readRGB(const nlohmann::json& src, RGBColor& dest);
};

inline std::unique_ptr<Config> cfg = std::make_unique<Config>();