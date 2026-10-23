#pragma once

#include "Globals.h"
#include <vector>
#include <thread>
#include <mutex>

struct RGBColor {
    int r, g, b;
};

struct GradientConfig {
    int numSteps = 500;
    int delayMs = 10;
    float startHue = 0.0f;
    float endHue = 1.0f;
    float saturation = 1.0f;
    float value = 1.0f;
};

class GradientManager {
  public:
    GradientManager();

    RGBColor getCurrentColor() const;
    void setConfig(const GradientConfig &newConfig);

  private:
    GradientConfig config;
    std::vector<RGBColor> colors;
    size_t currentIndex = 0;

    RGBColor hsvToRgb(float h, float s, float v);
    void generateGradient();
    mutable std::mutex mutex;
    std::jthread worker;
};

extern GradientManager gradient;
