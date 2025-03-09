#include "GradientManager.h"
#include <chrono>
#include <cmath>

GradientManager::GradientManager() {
    generateGradient();
    std::thread(&GradientManager::runGradientLoop, this).detach();
}

RGBColor GradientManager::hsvToRgb(float h, float s, float v) {
    float r, g, b;
    int i = static_cast<int>(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    default: r = v; g = t; b = p; break;
    }

    return { static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255) };
}

void GradientManager::generateGradient() {
    colors.clear();
    float hueRange = config.endHue - config.startHue;
    for (int i = 0; i < config.numSteps; ++i) {
        float t = static_cast<float>(i) / (config.numSteps - 1); // Normalized step
        float h = config.startHue + (hueRange * t);              // Interpolate hue
        colors.push_back(hsvToRgb(h, config.saturation, config.value));
    }
}

void GradientManager::runGradientLoop() {
    while (!globals::finish) {
        currentIndex = (currentIndex + 1) % colors.size();
        std::this_thread::sleep_for(std::chrono::milliseconds(config.delayMs));
    }
}

RGBColor GradientManager::getCurrentColor() const {
    return colors[currentIndex];
}

void GradientManager::setConfig(const GradientConfig& newConfig) {
    config = newConfig;
    generateGradient(); // Regenerate gradient with new settings
}

GradientManager gradient; // Definition of global instance