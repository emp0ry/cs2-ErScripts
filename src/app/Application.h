#pragma once
#include <chrono>

class Application {
  public:
    int run(bool preview = false);

  private:
    int exitKey = -1;
    std::chrono::steady_clock::time_point exitKeyChanged{};
    void pollExitKey();
};
