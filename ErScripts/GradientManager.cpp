#include "GradientManager.h"
#include <algorithm>
#include <chrono>
#include <cmath>

GradientManager::GradientManager() {
    generateGradient();
    worker = std::jthread([this](std::stop_token token) {
        while (!token.stop_requested()) {
            int delay;
            {
                std::lock_guard lock(mutex);
                currentIndex = (currentIndex + 1) % colors.size();
                delay = config.delayMs;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    });
}
RGBColor GradientManager::hsvToRgb(float h, float s, float v) {
    const float value = h * 6;
    const int sector = int(value);
    const float f = value - sector;
    const float p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    float r, g, b;
    switch (sector % 6) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    default:
        r = v;
        g = p;
        b = q;
        break;
    }
    return {int(r * 255), int(g * 255), int(b * 255)};
}
void GradientManager::generateGradient() {
    colors.clear();
    colors.reserve(config.numSteps);
    currentIndex = 0;
    for (int i = 0; i < config.numSteps; ++i) {
        const float t = float(i) / (config.numSteps - 1);
        colors.push_back(
            hsvToRgb(config.startHue + (config.endHue - config.startHue) * t, config.saturation, config.value));
    }
}
RGBColor GradientManager::getCurrentColor() const {
    std::lock_guard lock(mutex);
    return colors[currentIndex];
}
void GradientManager::setConfig(const GradientConfig &value) {
    std::lock_guard lock(mutex);
    config = value;
    config.numSteps = std::clamp(config.numSteps, 2, 2000);
    config.delayMs = std::clamp(config.delayMs, 1, 100);
    const auto unit = [](float v) { return std::isfinite(v) ? std::clamp(v, 0.f, 1.f) : 0.f; };
    config.startHue = unit(config.startHue);
    config.endHue = unit(config.endHue);
    config.saturation = unit(config.saturation);
    config.value = unit(config.value);
    generateGradient();
}
GradientManager gradient;
