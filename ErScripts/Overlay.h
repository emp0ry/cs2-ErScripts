#pragma once

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>

#include "ErScripts.h"
#include "ImagesShellCodes.h"

#include <dwmapi.h>
#include <d3d11.h>
#include <string>
#include <wincodec.h>
#include <comdef.h>
#include <chrono>
#include <ctime>
#include <Windows.h>

class Overlay {
    friend struct OverlayTestAccess;

  public:
    static std::atomic<float> g_ScrollDelta;

    void run();
    ~Overlay();

  private:
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
    ID3D11ShaderResourceView *settingsTexture = nullptr;
    ID3D11ShaderResourceView *reloadTexture = nullptr;

    bool isFinish = true;

    template <typename T> inline VOID SafeRelease(T *&p) noexcept;

    void LimitFPS(int fpsLimit) noexcept;

    bool CreateDeviceD3D() noexcept;
    void CleanupDeviceD3D() noexcept;
    void CreateRenderTarget() noexcept;
    void CleanupRenderTarget() noexcept;
    bool ResizeRenderTarget() noexcept;

    void Handler() noexcept;
    void Render() noexcept;
    void RenderWatermark() noexcept;
    void Menu() noexcept;

    FLOAT RenderText(ImDrawList *draw_list, ImFont *font, const std::string &text, const ImVec2 &position,
                     const float size, const ImColor &color, const bool centerX, const bool centerY, const bool outline,
                     const bool background) noexcept;
    void RenderCrosshair(const std::optional<SteamTools::Config> &ch, const crosshair::Frame &frame = {},
                         video::Size viewport = {});
    void CircularTimer(const ImVec2 &pos, const float min, const float max, const float &time, const int points_count,
                       const float radius, const float thickness, const bool top, const ImColor &color);
    ID3D11ShaderResourceView *LoadTextureFromPNGShellcode(unsigned char *icon_png, unsigned int icon_png_size);
    bool ImageButton(const char *str_id, ImTextureID imageTexture, const ImVec2 &size);

    void OverlayLoop() noexcept;
};
