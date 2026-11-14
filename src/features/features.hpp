#pragma once
#include <array>
#include <functional>
#include <optional>
#include <thread>
#include <vector>

class GameConsole;

class Features {
  public:
    explicit Features(GameConsole& console) : console_(console) {}
    ~Features();
    Features(const Features&) = delete;
    Features& operator=(const Features&) = delete;

    void autoAccept();
    void pixelTrigger();
    void crosshair();
    void bombTimer();
    void rgbCrosshair();
    void knifeSwitch();
    void autoPistol();
    void antiAfk();
    void cS2Binds();
    void killSay();
    void killSound();
    void roundStartAlert();
    void autoStop();
    void chatSpammer();

  private:
    GameConsole& console_;
    std::vector<std::thread> workers_;
    void startWorker(const char* name, std::function<void()> work);
    static std::optional<std::array<int, 3>> getPixelColor(int x, int y);
    static bool isColorSimilar(const std::array<int, 3>& color, const std::array<int, 3>& target, int range);
};
