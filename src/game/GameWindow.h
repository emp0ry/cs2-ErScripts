#pragma once
#include <windows.h>

namespace game {
    HWND window();
    bool hasFocus();
    bool cursorHidden();
    void windowBounds(int &width, int &height, int &x, int &y);
} // namespace game
