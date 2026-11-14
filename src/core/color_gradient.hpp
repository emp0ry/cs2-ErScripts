#pragma once

#include <chrono>
#include <mutex>
#include <vector>

struct RgbColor {
    int r, g, b;
    bool operator==(const RgbColor&) const = default;
};

struct GradientConfig {
    int numSteps = 500;
    int delayMs = 10;
    float startHue = 0.0f;
    float endHue = 1.0f;
    float saturation = 1.0f;
    float value = 1.0f;
    bool operator==(const GradientConfig&) const = default;
};

class ColorGradient {
  public:
    ColorGradient();

    RgbColor getCurrentColor(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) const;
    void setConfig(const GradientConfig& newConfig,
                   std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

  private:
    GradientConfig config_;
    std::vector<RgbColor> colors_;
    std::chrono::steady_clock::time_point started_;

    RgbColor hsvToRgb(float h, float s, float v);
    void generateGradient();
    mutable std::mutex mutex_;
};

ColorGradient& colorGradient();
