#include "ErScripts.h"

void ErScripts::PixelTrigger() {
    startWorker("PixelTrigger", [this]() {
        HDC hScreenDC = GetDC(nullptr);
        if (!hScreenDC)
            return;


        while (!globals::finish) {
            const auto settings = Config::snapshot();
            if (!settings->triggerState || !GetWindowState() || !GetCursorState()) {
                Sleep(1);
                continue;
            }

            const auto bounds = globals::getWindowBounds();
            const int width = bounds.width, height = bounds.height;

            int targetX1 = (width / 2) + bounds.x + settings->triggerDisplacement[0];
            int targetY1 = (height / 2) + bounds.y + settings->triggerDisplacement[1];

            int targetX2 = (width / 2) + bounds.x - settings->triggerDisplacement[0];
            int targetY2 = (height / 2) + bounds.y - settings->triggerDisplacement[1];

            if (settings->triggerBind) {
                if (GetAsyncKeyState(settings->triggerBind) & 0x8000) {

                    COLORREF baseColor1 = GetPixel(hScreenDC, targetX1, targetY1);
                    BYTE baseR1 = GetRValue(baseColor1);
                    BYTE baseG1 = GetGValue(baseColor1);
                    BYTE baseB1 = GetBValue(baseColor1);

                    COLORREF baseColor2 = GetPixel(hScreenDC, targetX2, targetY2);
                    BYTE baseR2 = GetRValue(baseColor2);
                    BYTE baseG2 = GetGValue(baseColor2);
                    BYTE baseB2 = GetBValue(baseColor2);

                    if (baseColor1 == CLR_INVALID || baseColor2 == CLR_INVALID) {
                        Sleep(1);
                        continue;
                    }

                    while (!globals::finish && (GetAsyncKeyState(settings->triggerBind) & 0x8000)) {
                        const auto current = Config::snapshot();
                        if (!current->triggerState || current->triggerBind != settings->triggerBind ||
                            globals::getWindowBounds() != bounds || !GetWindowState() || !GetCursorState())
                            break;

                        COLORREF currentColor1 = GetPixel(hScreenDC, targetX1, targetY1);
                        BYTE currentR1 = GetRValue(currentColor1);
                        BYTE currentG1 = GetGValue(currentColor1);
                        BYTE currentB1 = GetBValue(currentColor1);

                        COLORREF currentColor2 = GetPixel(hScreenDC, targetX2, targetY2);
                        BYTE currentR2 = GetRValue(currentColor2);
                        BYTE currentG2 = GetGValue(currentColor2);
                        BYTE currentB2 = GetBValue(currentColor2);

                        bool colorChanged = (abs(baseR1 - currentR1) > current->triggerThreshold ||
                                             abs(baseG1 - currentG1) > current->triggerThreshold ||
                                             abs(baseB1 - currentB1) > current->triggerThreshold) ||
                                            (abs(baseR2 - currentR2) > current->triggerThreshold ||
                                             abs(baseG2 - currentG2) > current->triggerThreshold ||
                                             abs(baseB2 - currentB2) > current->triggerThreshold);

                        if (currentColor1 == CLR_INVALID || currentColor2 == CLR_INVALID)
                            break;

                        if (colorChanged) {
                            if (current->triggerDelayMs)
                                std::this_thread::sleep_for(std::chrono::milliseconds(current->triggerDelayMs));
                            const auto latest = Config::snapshot();
                            if (globals::finish || !latest->triggerState ||
                                latest->triggerBind != settings->triggerBind || !GetWindowState() ||
                                !GetCursorState() || !(GetAsyncKeyState(settings->triggerBind) & 0x8000))
                                break;
                            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                            std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 32) + 16));
                            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::microseconds(15625));
        }

        ReleaseDC(nullptr, hScreenDC);
    });
}
