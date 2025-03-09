#pragma once

#include "Globals.h"
#include <vector>
#include <thread>

struct RGBColor {
    int r, g, b;
};

struct GradientConfig {
    int numSteps = 500;            // Number of steps in the gradient (smoothness)
    int delayMs = 10;              // Delay between color changes in milliseconds
    float startHue = 0.0f;         // Start hue (0.0 to 1.0)
    float endHue = 1.0f;           // End hue (0.0 to 1.0)
    float saturation = 1.0f;       // Saturation (0.0 to 1.0)
    float value = 1.0f;            // Value/Brightness (0.0 to 1.0)
};

class GradientManager {
public:
    GradientManager();

    RGBColor getCurrentColor() const;
    void setConfig(const GradientConfig& newConfig);

private:
    GradientConfig config;
    std::vector<RGBColor> colors;
    size_t currentIndex = 0;

    RGBColor hsvToRgb(float h, float s, float v);
    void generateGradient();
    void runGradientLoop();
};

extern GradientManager gradient; // Global instance