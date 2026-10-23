#include "config.h"
#include <algorithm>

namespace {
    const nlohmann::json &configValue(const nlohmann::json &root, std::initializer_list<const char *> path) {
        static const nlohmann::json empty;
        const auto *current = &root;
        for (const auto *key : path) {
            if (!current->is_object())
                return empty;
            const auto it = current->find(key);
            if (it == current->end())
                return empty;
            current = &*it;
        }
        return *current;
    }
} // namespace

void Config::load(const std::string &filename) {
    triggerState = autoStopState = angleBindState = autoPistolState = false;
    if (!std::filesystem::exists("configs"))
        std::filesystem::create_directory("configs");

    std::ifstream file("configs\\" + filename + ".json");
    if (!file.is_open())
        return;

    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::exception &e) {
        Logger::logWarning(std::format("Cannot load config: {}", e.what()));
        return;
    }
    if (!json.is_object()) {
        Logger::logWarning("Config must be a JSON object");
        return;
    }
    for (const auto *key :
         {"pixel-trigger", "auto-accept",  "bomb-timer",  "sniper-crosshair", "recoil-crosshair",  "rgb-crosshair",
          "keystrokes",    "knife-switch", "auto-pistol", "anti-afk",         "long-jump",         "jump-throw",
          "drop-bomb",     "self-kick",    "kill-say",    "kill-sound",       "round-start-alert", "auto-stop",
          "chat-spammer",  "angle-bind",   "watermark",   "fps-limiter",      "capture-bypass",    "gradient-manager",
          "er-scripts",    "vsync"}) {
        if (json.contains(key) && !json[key].is_object()) {
            Logger::logWarning("Invalid config section");
            return;
        }
    }

    read(configValue(json, {"pixel-trigger", "bind"}), triggerBind);
    readArray(configValue(json, {"pixel-trigger", "displacement"}), triggerDisplacement);
    read(configValue(json, {"pixel-trigger", "threshold"}), triggerThreshold);
    read(configValue(json, {"pixel-trigger", "delay"}), triggerDelayMs);

    read(configValue(json, {"auto-accept", "state"}), autoAcceptState);
    read(configValue(json, {"auto-accept", "waiting-time"}), autoAcceptWaitingTime);

    read(configValue(json, {"bomb-timer", "state"}), bombTimerState);
    read(configValue(json, {"bomb-timer", "scale"}), bombTimerScale);
    read(configValue(json, {"bomb-timer", "gradient", "state"}), bombTimerGradientState);
    read(configValue(json, {"bomb-timer", "transparency"}), bombTimerTransparency);
    readArray(configValue(json, {"bomb-timer", "pos"}), bombTimerPos);

    read(configValue(json, {"sniper-crosshair", "state"}), sniperCrosshairState);

    read(configValue(json, {"recoil-crosshair", "state"}), recoilCrosshairState);

    read(configValue(json, {"rgb-crosshair", "state"}), rgbCrosshairState);

    read(configValue(json, {"keystrokes", "state"}), keystrokesState);
    read(configValue(json, {"keystrokes", "scale"}), keystrokesScale);
    read(configValue(json, {"keystrokes", "gradient", "state"}), keystrokesGradientState);
    read(configValue(json, {"keystrokes", "animation", "speed"}), keystrokesAnimationSpeed);
    readRGB(configValue(json, {"keystrokes", "pressed", "color"}), keystrokesPressedColor);
    readRGB(configValue(json, {"keystrokes", "released", "color"}), keystrokesReleasedColor);
    readArray(configValue(json, {"keystrokes", "pos"}), keystrokesPos);
    read(configValue(json, {"keystrokes", "transparency"}), keystrokesTransparency);

    read(configValue(json, {"knife-switch", "state"}), knifeSwitchState);

    read(configValue(json, {"anti-afk", "state"}), antiAfkState);

    read(configValue(json, {"long-jump", "bind"}), longJumpBind);

    read(configValue(json, {"jump-throw", "bind"}), jumpThrowBind);

    read(configValue(json, {"drop-bomb", "bind"}), dropBombBind);

    read(configValue(json, {"self-kick", "bind"}), selfKickBind);

    read(configValue(json, {"kill-say", "state"}), killSayState);
    read(configValue(json, {"kill-say", "text"}), killSayText);

    read(configValue(json, {"kill-sound", "state"}), killSoundState);
    read(configValue(json, {"kill-sound", "volume"}), killSoundVolume);
    read(configValue(json, {"kill-sound", "file-name"}), killSoundFileName);

    read(configValue(json, {"round-start-alert", "state"}), roundStartAlertState);
    read(configValue(json, {"round-start-alert", "volume"}), roundStartAlertVolume);
    read(configValue(json, {"round-start-alert", "file-name"}), roundStartAlertFileName);

    read(configValue(json, {"auto-stop", "bind"}), autoStopBind);
    read(configValue(json, {"auto-stop", "toggle", "state"}), autoStopToggleState);

    read(configValue(json, {"chat-spammer", "state"}), chatSpammerState);
    read(configValue(json, {"chat-spammer", "bind"}), chatSpammerBind);
    read(configValue(json, {"chat-spammer", "text"}), chatSpammerText);
    if (chatSpammerText == "Enhance Your Skills with emp0ry.github.io/cs2-ErScripts/" ||
        chatSpammerText == "erscripts.emp0ry.com")
        chatSpammerText = defaultChatMessage;

    read(configValue(json, {"angle-bind", "bind"}), angleBindBind);
    read(configValue(json, {"angle-bind", "degree"}), angleBindDegree);

    read(configValue(json, {"watermark", "state"}), watermarkState);
    read(configValue(json, {"watermark", "gradient", "state"}), watermarkGradientState);
    read(configValue(json, {"watermark", "transparency"}), watermarkTransparency);
    read(configValue(json, {"watermark", "ping-update-rate"}), watermarkPingUpdateRate);

    read(configValue(json, {"fps-limiter", "state"}), fpsLimiterState);
    read(configValue(json, {"fps-limiter", "fps"}), fpsLimiter);

    read(configValue(json, {"capture-bypass", "state"}), captureBypassState);

    read(configValue(json, {"gradient-manager", "num-steps"}), gradient.numSteps);
    read(configValue(json, {"gradient-manager", "delay-ms"}), gradient.delayMs);
    read(configValue(json, {"gradient-manager", "start-hue"}), gradient.startHue);
    read(configValue(json, {"gradient-manager", "end-hue]"}), gradient.endHue);
    read(configValue(json, {"gradient-manager", "end-hue"}), gradient.endHue);
    read(configValue(json, {"gradient-manager", "saturation"}), gradient.saturation);
    read(configValue(json, {"gradient-manager", "value"}), gradient.value);

    read(configValue(json, {"er-scripts", "menu", "bind"}), erScriptsMenuBind);
    read(configValue(json, {"er-scripts", "exit", "bind"}), erScriptsExitBind);

    read(configValue(json, {"vsync", "state"}), vsyncState);
    fpsLimiter = std::clamp(fpsLimiter, 1, 1000);
    watermarkPingUpdateRate = std::clamp(watermarkPingUpdateRate, 1, 60);
    gradient.numSteps = std::clamp(gradient.numSteps, 2, 2000);
    gradient.delayMs = std::clamp(gradient.delayMs, 1, 100);
    bombTimerScale = std::clamp(bombTimerScale, .5f, 2.f);
    keystrokesScale = std::clamp(keystrokesScale, .5f, 2.f);
}

void Config::save(const std::string &filename) const {
    nlohmann::ordered_json json;

    json["pixel-trigger"]["state"] = triggerState;
    json["pixel-trigger"]["bind"] = triggerBind;
    json["pixel-trigger"]["displacement"] = triggerDisplacement;
    json["pixel-trigger"]["threshold"] = triggerThreshold;
    json["pixel-trigger"]["delay"] = triggerDelayMs;

    json["auto-accept"]["state"] = autoAcceptState;
    json["auto-accept"]["waiting-time"] = autoAcceptWaitingTime;

    json["bomb-timer"]["state"] = bombTimerState;
    json["bomb-timer"]["scale"] = bombTimerScale;
    json["bomb-timer"]["gradient"]["state"] = bombTimerGradientState;
    json["bomb-timer"]["transparency"] = bombTimerTransparency;
    json["bomb-timer"]["pos"] = bombTimerPos;

    json["sniper-crosshair"]["state"] = sniperCrosshairState;

    json["rgb-crosshair"]["state"] = rgbCrosshairState;

    json["recoil-crosshair"]["state"] = recoilCrosshairState;

    json["keystrokes"]["state"] = keystrokesState;
    json["keystrokes"]["scale"] = keystrokesScale;
    json["keystrokes"]["gradient"]["state"] = keystrokesGradientState;
    json["keystrokes"]["animation"]["speed"] = keystrokesAnimationSpeed;
    json["keystrokes"]["pressed"]["color"] = {keystrokesPressedColor.r, keystrokesPressedColor.g,
                                              keystrokesPressedColor.b};
    json["keystrokes"]["released"]["color"] = {keystrokesReleasedColor.r, keystrokesReleasedColor.g,
                                               keystrokesReleasedColor.b};
    json["keystrokes"]["pos"] = keystrokesPos;
    json["keystrokes"]["transparency"] = keystrokesTransparency;

    json["knife-switch"]["state"] = knifeSwitchState;

    json["auto-pistol"]["state"] = autoPistolState;

    json["anti-afk"]["state"] = antiAfkState;

    json["long-jump"]["bind"] = longJumpBind;

    json["jump-throw"]["bind"] = jumpThrowBind;

    json["drop-bomb"]["bind"] = dropBombBind;

    json["self-kick"]["bind"] = selfKickBind;

    json["kill-say"]["state"] = killSayState;
    json["kill-say"]["text"] = killSayText;

    json["kill-sound"]["state"] = killSoundState;
    json["kill-sound"]["volume"] = killSoundVolume;
    json["kill-sound"]["file-name"] = killSoundFileName;

    json["round-start-alert"]["state"] = roundStartAlertState;
    json["round-start-alert"]["volume"] = roundStartAlertVolume;
    json["round-start-alert"]["file-name"] = roundStartAlertFileName;

    json["auto-stop"]["state"] = autoStopState;
    json["auto-stop"]["bind"] = autoStopBind;
    json["auto-stop"]["toggle"]["state"] = autoStopToggleState;

    json["chat-spammer"]["state"] = chatSpammerState;
    json["chat-spammer"]["bind"] = chatSpammerBind;
    json["chat-spammer"]["text"] = chatSpammerText;

    json["angle-bind"]["state"] = angleBindState;
    json["angle-bind"]["bind"] = angleBindBind;
    json["angle-bind"]["degree"] = angleBindDegree;

    json["watermark"]["state"] = watermarkState;
    json["watermark"]["gradient"]["state"] = watermarkGradientState;
    json["watermark"]["transparency"] = watermarkTransparency;
    json["watermark"]["ping-update-rate"] = watermarkPingUpdateRate;

    json["fps-limiter"]["state"] = fpsLimiterState;
    json["fps-limiter"]["fps"] = fpsLimiter;

    json["capture-bypass"]["state"] = captureBypassState;

    json["gradient-manager"]["num-steps"] = gradient.numSteps;
    json["gradient-manager"]["delay-ms"] = gradient.delayMs;
    json["gradient-manager"]["start-hue"] = gradient.startHue;
    json["gradient-manager"]["end-hue"] = gradient.endHue;
    json["gradient-manager"]["saturation"] = gradient.saturation;
    json["gradient-manager"]["value"] = gradient.value;

    json["vsync"]["state"] = vsyncState;

    json["er-scripts"]["menu"]["bind"] = erScriptsMenuBind;
    json["er-scripts"]["exit"]["bind"] = erScriptsExitBind;

    if (!std::filesystem::exists("configs"))
        std::filesystem::create_directory("configs");

    std::ofstream file("configs\\" + filename + ".json");
    if (file.is_open()) {
        file << json.dump(4);
    }
}

void Config::readArray(const nlohmann::json &src, float (&dest)[2]) {
    try {
        if (!src.is_null() && src.is_array() && src.size() == 2) {
            dest[0] = src[0].get<float>();
            dest[1] = src[1].get<float>();
        }
    } catch (const std::exception &e) {
        Logger::logWarning(std::format("Config Array Error: {}", e.what()));
    }
}

void Config::readArray(const nlohmann::json &src, int (&dest)[2]) {
    try {
        if (!src.is_null() && src.is_array() && src.size() == 2) {
            dest[0] = src[0].get<int>();
            dest[1] = src[1].get<int>();
        }
    } catch (const std::exception &e) {
        Logger::logWarning(std::format("Config Array Error: {}", e.what()));
    }
}

void Config::readRGB(const nlohmann::json &src, RGBColor &dest) {
    try {
        if (!src.is_null() && src.is_array() && src.size() == 3) {
            dest.r = src[0].get<int>();
            dest.g = src[1].get<int>();
            dest.b = src[2].get<int>();
        }
    } catch (const std::exception &e) {
        Logger::logWarning(std::format("Config Array Error: {}", e.what()));
    }
}
