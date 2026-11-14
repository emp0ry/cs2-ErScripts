#pragma once
#include <random>
#include <vector>

namespace autostop {
    int randomDelay(std::vector<int> &recent, int minimum, int maximum, std::mt19937 &rng);
}
