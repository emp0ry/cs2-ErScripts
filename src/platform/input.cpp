#include "platform/input.hpp"

#include <memory>
#include <type_traits>
#include <windows.h>

bool input::setKey(int key, bool isPressed) noexcept {
    static const std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&FreeLibrary)> win32u(
        LoadLibraryExW(L"win32u.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32), FreeLibrary);
    static const auto ntUserSendInput = reinterpret_cast<UINT(WINAPI*)(UINT, LPINPUT, INT)>(
        win32u ? GetProcAddress(win32u.get(), "NtUserSendInput") : nullptr);

    INPUT input{};

    if (key == VK_LBUTTON || key == VK_RBUTTON || key == VK_MBUTTON || key == VK_XBUTTON1 || key == VK_XBUTTON2) {
        input.type = INPUT_MOUSE;

        if (key == VK_LBUTTON)
            input.mi.dwFlags = isPressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        else if (key == VK_RBUTTON)
            input.mi.dwFlags = isPressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        else if (key == VK_MBUTTON)
            input.mi.dwFlags = isPressed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        else {
            input.mi.dwFlags = isPressed ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
            input.mi.mouseData = (key == VK_XBUTTON1) ? XBUTTON1 : XBUTTON2;
        }
    } else {
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = static_cast<WORD>(key);
        input.ki.dwFlags = isPressed ? 0 : KEYEVENTF_KEYUP;
    }

    if (ntUserSendInput) {
        UINT r = ntUserSendInput(1, &input, sizeof(INPUT));
        if (r == 1)
            return true;
    }

    return SendInput(1, &input, sizeof(INPUT)) == 1;
}
