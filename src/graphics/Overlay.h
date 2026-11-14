#pragma once

#include "graphics/CrosshairGeometry.h"
#include "game/GameVideo.h"
#include <imgui.h>
#include <windows.h>
#include <d3d11.h>
#include <atomic>
#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <thread>

class Overlay {
    friend struct OverlayTestAccess;

  public:
    static std::atomic<float> g_ScrollDelta;

    void run();
    ~Overlay();

  private:
    static constexpr wchar_t windowClassName[] = L"GDI+ Hook Window Class";
    std::thread renderThread;
    HWND window_handle = nullptr;
    ID3D11Device *g_pd3dDevice = nullptr;
    ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
    IDXGISwapChain *g_pSwapChain = nullptr;
    bool g_SwapChainOccluded = false;
    HRESULT resizeFailure = S_OK;
    ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

    ImDrawList *draw_list = nullptr;
    ImFont *menu_font = nullptr;
    ImFont *arial_font = nullptr;
    ImFont *bold_font = nullptr;

    ID3D11ShaderResourceView *bombTexture = nullptr;
    std::chrono::steady_clock::time_point frameDeadline{}, changeTime{};
    bool showChat = false, prevMenuState = false, curMenuState = false, isDelayActive = false;
    int prevMenuBind = -1;
    int width_old = 0, height_old = 0, posX_old = 0, posY_old = 0;
    std::optional<bool> requestedCaptureState;
    std::array<bool, 256> keyState = [] {
        std::array<bool, 256> keys;
        keys.fill(true);
        return keys;
    }();

    template <typename T> inline VOID SafeRelease(T *&p) noexcept;

    void LimitFPS(int fpsLimit);

    bool CreateDeviceD3D();
    void CleanupDeviceD3D() noexcept;
    void CreateRenderTarget() noexcept;
    void CleanupRenderTarget() noexcept;
    bool ResizeRenderTarget();

    void Handler();
    void Render();
    void RenderWatermark();
    void Menu();

    FLOAT RenderText(ImDrawList *draw_list, ImFont *font, const std::string &text, const ImVec2 &position,
                     const float size, const ImColor &color, const bool centerX, const bool centerY, const bool outline,
                     const bool background);
    void RenderCrosshair(const std::optional<crosshair::Settings> &ch, const crosshair::Frame &frame = {},
                         video::Size viewport = {});
    void CircularTimer(const ImVec2 &pos, const float min, const float max, const float &time, const int points_count,
                       const float radius, const float thickness, const bool top, const ImColor &color);
    ID3D11ShaderResourceView *LoadTextureFromPNGShellcode(unsigned char *icon_png, unsigned int icon_png_size);

    void OverlayLoop();
};
