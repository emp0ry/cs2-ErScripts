#pragma once
#include <functional>
#include <array>
#include <optional>
#include <thread>
#include <vector>

class GameConsole;

class Features {
  public:
    explicit Features(GameConsole &console) : console(console) {}
    ~Features();
    Features(const Features &) = delete;
    Features &operator=(const Features &) = delete;

    void AutoAccept();
    void PixelTrigger();
    void Crosshair();
    void BombTimer();
    void RGBCrosshair();
    void KnifeSwitch();
    void AutoPistol();
    void AntiAfk();
    void CS2Binds();
    void KillSay();
    void KillSound();
    void RoundStartAlert();
    void AutoStop();
    void ChatSpammer();

  private:
    GameConsole &console;
    std::vector<std::thread> workers;
    void startWorker(const char *name, std::function<void()> work);
    static std::optional<std::array<int, 3>> GetPixelColor(int x, int y);
    static bool isColorSimilar(const std::array<int, 3> &color, const std::array<int, 3> &target, int range);
};
