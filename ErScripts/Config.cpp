#include "config.h"

void Config::load(const std::string& filename) {
    if (!std::filesystem::exists("configs"))
        std::filesystem::create_directory("configs");

    std::ifstream file("configs\\" + filename + ".json");
    if (!file.is_open()) return;

    nlohmann::json json;
    file >> json;

    /* Pixel Trigger */
    read(json["pixel-trigger"]["state"], triggerState);
    read(json["pixel-trigger"]["bind"], triggerBind);
    readArray(json["pixel-trigger"]["displacement"], triggerDisplacement);
    read(json["pixel-trigger"]["threshold"], triggerThreshold);
    read(json["pixel-trigger"]["delay"], triggerDelayMs);

    /* Auto Accept */
    read(json["auto-accept"]["state"], autoAcceptState);
    read(json["auto-accept"]["waiting-time"], autoAcceptWaitingTime);

    /* Bomb Timer */
    read(json["bomb-timer"]["state"], bombTimerState);
    read(json["bomb-timer"]["scale"], bombTimerScale);
    read(json["bomb-timer"]["gradient"]["state"], bombTimerGradientState);
    read(json["bomb-timer"]["transparency"], bombTimerTransparency);
    readArray(json["bomb-timer"]["pos"], bombTimerPos);

    /* Sniper Crosshair */
    read(json["sniper-crosshair"]["state"], sniperCrosshairState);

    /* RGB Crosshair */
    read(json["rgb-crosshair"]["state"], rgbCrosshairState);

    /* Keystrokes */
    read(json["keystrokes"]["state"], keystrokesState);
    read(json["keystrokes"]["scale"], keystrokesScale);
    read(json["keystrokes"]["gradient"]["state"], keystrokesGradientState);
    read(json["keystrokes"]["animation"]["speed"], keystrokesAnimationSpeed);
    readRGB(json["keystrokes"]["pressed"]["color"], keystrokesPressedColor);
    readRGB(json["keystrokes"]["released"]["color"], keystrokesReleasedColor);
    readArray(json["keystrokes"]["pos"], keystrokesPos);

    /* Keystrokes */
    read(json["knife-switch"]["state"], knifeSwitchState);

    /* Auto pistol */
    read(json["auto-pistol"]["state"], autoPistolState);

    /* Anti Afk */
    read(json["anti-afk"]["state"], antiAfkState);

    /* Long Jump */
    read(json["long-jump"]["bind"], longJumpBind);

    /* Jump Throw */
    read(json["jump-throw"]["bind"], jumpThrowBind);

    /* Drop Bomb */
    read(json["drop-bomb"]["bind"], dropBombBind);

    /* Self Kick */
    read(json["self-kick"]["bind"], selfKickBind);

    /* Kill Say */
    read(json["kill-say"]["state"], killSayState);
    read(json["kill-say"]["text"], killSayText);

    /* Kill Sound */
    read(json["kill-sound"]["state"], killSoundState);
    read(json["kill-sound"]["volume"], killSoundVolume);
    read(json["kill-sound"]["file-name"], killSoundFileName);

    /* Round Start Alert */
    read(json["round-start-alert"]["state"], roundStartAlertState);
    read(json["round-start-alert"]["volume"], roundStartAlertVolume);
    read(json["round-start-alert"]["file-name"], roundStartAlertFileName);

    /* Auto Stop */
    read(json["auto-stop"]["state"], autoStopState);
    read(json["auto-stop"]["bind"], autoStopBind);
    read(json["auto-stop"]["toggle"]["state"], autoStopToggleState);

    /* Watermark */
    read(json["watermark"]["state"], watermarkState);
    read(json["watermark"]["gradient"]["state"], watermarkGradientState);
    read(json["watermark"]["transparency"], watermarkTransparency);
    read(json["watermark"]["ping-update-rate"], watermarkPingUpdateRate);

    /* FPS Limiter */
    read(json["fps-limiter"]["state"], fpsLimiterState);
    read(json["fps-limiter"]["fps"], fpsLimiter);

    /* Capture Bypass */
    read(json["capture-bypass"]["state"], captureBypassState);

    /* Gradient Manager */
    read(json["gradient-manager"]["num-steps"], gradient.numSteps);
    read(json["gradient-manager"]["delay-ms"], gradient.delayMs);
    read(json["gradient-manager"]["start-hue"], gradient.startHue);
    read(json["gradient-manager"]["end-hue]"], gradient.endHue);
    read(json["gradient-manager"]["saturation"], gradient.saturation);
    read(json["gradient-manager"]["value"], gradient.value);

    /* Vsync */
    read(json["vsync"]["state"], vsyncState);
}

void Config::save(const std::string& filename) const {
    nlohmann::ordered_json json;

    /* Pixel Trigger */
    json["pixel-trigger"]["state"] = triggerState;
    json["pixel-trigger"]["bind"] = triggerBind;
    json["pixel-trigger"]["displacement"] = triggerDisplacement;
    json["pixel-trigger"]["threshold"] = triggerThreshold;
    json["pixel-trigger"]["delay"] = triggerDelayMs;

    /* Auto Accept */
    json["auto-accept"]["state"] = autoAcceptState;
    json["auto-accept"]["waiting-time"] = autoAcceptWaitingTime;

    /* Bomb Timer */
    json["bomb-timer"]["state"] = bombTimerState;
    json["bomb-timer"]["scale"] = bombTimerScale;
    json["bomb-timer"]["gradient"]["state"] = bombTimerGradientState;
    json["bomb-timer"]["transparency"] = bombTimerTransparency;
    json["bomb-timer"]["pos"] = bombTimerPos;

    /* Sniper Crosshair */
    json["sniper-crosshair"]["state"] = sniperCrosshairState;

    /* RGB Crosshair */
    json["rgb-crosshair"]["state"] = rgbCrosshairState;

    /* Keystrokes */
    json["keystrokes"]["state"] = keystrokesState;
    json["keystrokes"]["scale"] = keystrokesScale;
    json["keystrokes"]["gradient"]["state"] = keystrokesGradientState;
    json["keystrokes"]["animation"]["speed"] = keystrokesAnimationSpeed;
    json["keystrokes"]["pressed"]["color"] = { keystrokesPressedColor.r, keystrokesPressedColor.g, keystrokesPressedColor.b };
    json["keystrokes"]["released"]["color"] = { keystrokesReleasedColor.r, keystrokesReleasedColor.g, keystrokesReleasedColor.b };
    json["keystrokes"]["pos"] = keystrokesPos;

    /* Keystrokes */
    json["knife-switch"]["state"] = knifeSwitchState;

    /* Auto pistol */
    json["auto-pistol"]["state"] = autoPistolState;

    /* Anti Afk */
    json["anti-afk"]["state"] = antiAfkState;

    /* Long Jump */
    json["long-jump"]["bind"] = longJumpBind;

    /* Jump Throw */
    json["jump-throw"]["bind"] = jumpThrowBind;

    /* Drop Bomb */
    json["drop-bomb"]["bind"] = dropBombBind;

    /* Self Kick */
    json["self-kick"]["bind"] = selfKickBind;

    /* Kill Say */
    json["kill-say"]["state"] = killSayState;
    json["kill-say"]["text"] = killSayText;

    /* Kill Sound */
    json["kill-sound"]["state"] = killSoundState;
    json["kill-sound"]["volume"] = killSoundVolume;
    json["kill-sound"]["file-name"] = killSoundFileName;

    /* Round Start Alert */
    json["round-start-alert"]["state"] = roundStartAlertState;
    json["round-start-alert"]["volume"] = roundStartAlertVolume;
    json["round-start-alert"]["file-name"] = roundStartAlertFileName;

    /* Auto Stop */
    json["auto-stop"]["state"] = autoStopState;
    json["auto-stop"]["bind"] = autoStopBind;
    json["auto-stop"]["toggle"]["state"] = autoStopToggleState;

    /* Watermark */
    json["watermark"]["state"] = watermarkState;
    json["watermark"]["gradient"]["state"] = watermarkGradientState;
    json["watermark"]["transparency"] = watermarkTransparency;
    json["watermark"]["ping-update-rate"] = watermarkPingUpdateRate;

    /* FPS Limiter */
    json["fps-limiter"]["state"] = fpsLimiterState;
    json["fps-limiter"]["fps"] = fpsLimiter;

    /* Capture Bypass */
    json["capture-bypass"]["state"] = captureBypassState;

    /* Gradient Manager */
    json["gradient-manager"]["num-steps"] = gradient.numSteps;
    json["gradient-manager"]["delay-ms"] = gradient.delayMs;
    json["gradient-manager"]["start-hue"] = gradient.startHue;
    json["gradient-manager"]["end-hue]"] = gradient.endHue;
    json["gradient-manager"]["saturation"] = gradient.saturation;
    json["gradient-manager"]["value"] = gradient.value;

    /* Vsync */
    json["vsync"]["state"] = vsyncState;

    if (!std::filesystem::exists("configs"))
        std::filesystem::create_directory("configs");

    std::ofstream file("configs\\" + filename + ".json");
    if (file.is_open()) {
        file << json.dump(4); // Pretty print with 4 spaces indentation
    }
}

void Config::readArray(const nlohmann::json& src, float(&dest)[2]) {
    try {
        if (!src.is_null() && src.is_array() && src.size() == 2) {
            dest[0] = src[0].get<float>();
            dest[1] = src[1].get<float>();
        }
    }
    catch (const std::exception& e) {
        Logger::logWarning(std::format("Config Array Error: {}", e.what()));
    }
}

void Config::readArray(const nlohmann::json& src, int(&dest)[2]) {
    try {
        if (!src.is_null() && src.is_array() && src.size() == 2) {
            dest[0] = src[0].get<int>();
            dest[1] = src[1].get<int>();
        }
    }
    catch (const std::exception& e) {
        Logger::logWarning(std::format("Config Array Error: {}", e.what()));
    }
}

void Config::readRGB(const nlohmann::json& src, RGBColor& dest) {
    try {
        if (!src.is_null() && src.is_array() && src.size() == 3) {
            dest.r = src[0].get<int>();
            dest.g = src[1].get<int>();
            dest.b = src[2].get<int>();
        }
    }
    catch (const std::exception& e) {
        Logger::logWarning(std::format("Config Array Error: {}", e.what()));
    }
}