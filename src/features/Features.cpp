#include "features/Features.h"
#include "core/Globals.h"
#include "core/Logger.h"
#include <cstdlib>
#include <format>

Features::~Features() {
    globals::finish = true;
    for (auto &worker : workers)
        if (worker.joinable())
            worker.join();
}

void Features::startWorker(const char *name, std::function<void()> work) {
    workers.emplace_back([name, work = std::move(work)] {
        try {
            work();
        } catch (const std::exception &error) {
            Logger::logWarning(std::format("{} stopped: {}", name, error.what()));
        }
    });
}

std::optional<std::array<int, 3>> Features::GetPixelColor(int x, int y) {
    HDC dc = GetDC(nullptr);
    if (!dc)
        return std::nullopt;
    const COLORREF color = GetPixel(dc, x, y);
    ReleaseDC(nullptr, dc);
    if (color == CLR_INVALID)
        return std::nullopt;
    return std::array<int, 3>{GetRValue(color), GetGValue(color), GetBValue(color)};
}

bool Features::isColorSimilar(const std::array<int, 3> &color, const std::array<int, 3> &targetColor, int range) {
    return std::abs(color[0] - targetColor[0]) <= range && std::abs(color[1] - targetColor[1]) <= range &&
           std::abs(color[2] - targetColor[2]) <= range;
}
