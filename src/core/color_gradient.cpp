#include "core/color_gradient.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

ColorGradient::ColorGradient() {
    generateGradient();
    started_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(config_.delayMs);
}
RgbColor ColorGradient::hsvToRgb(float h, float s, float v) {
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
void ColorGradient::generateGradient() {
    colors_.clear();
    colors_.reserve(config_.numSteps);
    for (int i = 0; i < config_.numSteps; ++i) {
        const float t = float(i) / (config_.numSteps - 1);
        colors_.push_back(
            hsvToRgb(config_.startHue + (config_.endHue - config_.startHue) * t, config_.saturation, config_.value));
    }
}
RgbColor ColorGradient::getCurrentColor(std::chrono::steady_clock::time_point now) const {
    std::lock_guard lock(mutex_);
    const auto elapsed = std::max(now - started_, std::chrono::steady_clock::duration::zero());
    const auto steps = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / config_.delayMs;
    return colors_[static_cast<std::size_t>(steps) % colors_.size()];
}
void ColorGradient::setConfig(const GradientConfig& value, std::chrono::steady_clock::time_point now) {
    std::lock_guard lock(mutex_);
    config_ = value;
    config_.numSteps = std::clamp(config_.numSteps, 2, 2000);
    config_.delayMs = std::clamp(config_.delayMs, 1, 100);
    const auto unit = [](float v) { return std::isfinite(v) ? std::clamp(v, 0.f, 1.f) : 0.f; };
    config_.startHue = unit(config_.startHue);
    config_.endHue = unit(config_.endHue);
    config_.saturation = unit(config_.saturation);
    config_.value = unit(config_.value);
    generateGradient();
    started_ = now;
}
ColorGradient& colorGradient() {
    static ColorGradient gradient;
    return gradient;
}
