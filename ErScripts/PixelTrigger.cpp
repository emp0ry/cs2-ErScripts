#include "CS2Functions.h"

void CS2Functions::PixelTrigger() {
    std::thread([this]() {
        HDC hScreenDC = GetDC(nullptr);
        if (!hScreenDC) return;

        // Initial screen dimensions
        int width = globals::width;
        int height = globals::height;

        while (!globals::finish) {
            if (!cfg->triggerState) {
                Sleep(1); // Minimal sleep when inactive
                continue;
            }

            // Update dimensions if changed
            if (width != globals::width || height != globals::height) {
                width = globals::width;
                height = globals::height;
            }

            // First coordinate
            int targetX1 = (width / 2) + globals::posX + cfg->triggerDisplacement[0];
            int targetY1 = (height / 2) + globals::posY + cfg->triggerDisplacement[1];

            // Second coordinate (inverted displacements)
            int targetX2 = (width / 2) + globals::posX - cfg->triggerDisplacement[0];
            int targetY2 = (height / 2) + globals::posY - cfg->triggerDisplacement[1];

            if (GetAsyncKeyState(cfg->triggerBind) & 0x8000) {
                // Get initial colors for both points
                COLORREF baseColor1 = GetPixel(hScreenDC, targetX1, targetY1);
                BYTE baseR1 = GetRValue(baseColor1);
                BYTE baseG1 = GetGValue(baseColor1);
                BYTE baseB1 = GetBValue(baseColor1);

                COLORREF baseColor2 = GetPixel(hScreenDC, targetX2, targetY2);
                BYTE baseR2 = GetRValue(baseColor2);
                BYTE baseG2 = GetGValue(baseColor2);
                BYTE baseB2 = GetBValue(baseColor2);

                while (GetAsyncKeyState(cfg->triggerBind) & 0x8000) {
                    // Get current colors for both points
                    COLORREF currentColor1 = GetPixel(hScreenDC, targetX1, targetY1);
                    BYTE currentR1 = GetRValue(currentColor1);
                    BYTE currentG1 = GetGValue(currentColor1);
                    BYTE currentB1 = GetBValue(currentColor1);

                    COLORREF currentColor2 = GetPixel(hScreenDC, targetX2, targetY2);
                    BYTE currentR2 = GetRValue(currentColor2);
                    BYTE currentG2 = GetGValue(currentColor2);
                    BYTE currentB2 = GetBValue(currentColor2);

                    // Check color difference for either point
                    bool colorChanged =
                        (abs(baseR1 - currentR1) > cfg->triggerThreshold ||
                            abs(baseG1 - currentG1) > cfg->triggerThreshold ||
                            abs(baseB1 - currentB1) > cfg->triggerThreshold) ||
                        (abs(baseR2 - currentR2) > cfg->triggerThreshold ||
                            abs(baseG2 - currentG2) > cfg->triggerThreshold ||
                            abs(baseB2 - currentB2) > cfg->triggerThreshold);

                    if (colorChanged) {
                        if (cfg->triggerDelayMs) std::this_thread::sleep_for(std::chrono::milliseconds(cfg->triggerDelayMs));
                        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                        std::this_thread::sleep_for(std::chrono::milliseconds((rand() % 32) + 16));
                        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                    }
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        ReleaseDC(nullptr, hScreenDC);
        }).detach();
}