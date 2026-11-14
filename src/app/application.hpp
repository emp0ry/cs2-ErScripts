#pragma once
#include <chrono>

class Application {
  public:
    int run(bool preview = false);

  private:
    int exitKey_ = -1;
    std::chrono::steady_clock::time_point exitKeyChanged_{};
    void pollExitKey();
};
