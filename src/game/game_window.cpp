#include "game/game_window.hpp"

#include <atomic>
#include <chrono>

namespace {
    std::atomic<HWND> hwnd = nullptr;
}
HWND game::window() {
    const auto current = FindWindowW(nullptr, L"Counter-Strike 2");
    hwnd = current;
    return current;
}
bool game::cursorHidden() {
    CURSORINFO cinfo = {sizeof(CURSORINFO)};
    if (!GetCursorInfo(&cinfo))
        return false;
    return LOWORD(reinterpret_cast<ULONG_PTR>(cinfo.hCursor)) == 0;
}

bool game::hasFocus() {
    const auto target = hwnd.load();
    return target && GetForegroundWindow() == target;
}

void game::windowBounds(int& width, int& height, int& posX, int& posY) {
    auto target = hwnd.load();
    static auto nextWindowCheck = std::chrono::steady_clock::time_point{};
    const auto now = std::chrono::steady_clock::now();
    if (!IsWindow(target) || !IsWindowVisible(target) || now >= nextWindowCheck) {
        nextWindowCheck = now + std::chrono::milliseconds(250);
        if (const auto current = FindWindowA(nullptr, "Counter-Strike 2")) {
            target = current;
            hwnd = current;
        }
    }
    RECT rect{};
    POINT point{};
    if (IsIconic(target) || !GetClientRect(target, &rect) || !ClientToScreen(target, &point)) {
        width = height = 0;
        return;
    }
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    posX = point.x;
    posY = point.y;
}
