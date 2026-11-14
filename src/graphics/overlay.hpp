#pragma once

#include "game/game_video.hpp"
#include "graphics/crosshair_geometry.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <d3d11.h>
#include <optional>
#include <string>
#include <thread>
#include <windows.h>

#include <imgui.h>

class Overlay {
    friend struct OverlayTestAccess;

  public:
    static std::atomic<float> scrollDelta;

    void run();
    ~Overlay();

  private:
    static constexpr wchar_t windowClassName[] = L"GDI+ Hook Window Class";
    std::thread renderThread_;
    HWND windowHandle_ = nullptr;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* deviceContext_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    bool swapChainOccluded_ = false;
    HRESULT resizeFailure_ = S_OK;
    ID3D11RenderTargetView* renderTarget_ = nullptr;

    ImDrawList* drawList_ = nullptr;
    ImFont* menuFont_ = nullptr;
    ImFont* arialFont_ = nullptr;
    ImFont* boldFont_ = nullptr;

    ID3D11ShaderResourceView* bombTexture_ = nullptr;
    std::chrono::steady_clock::time_point frameDeadline_{}, changeTime_{};
    bool showChat_ = false, prevMenuState_ = false, curMenuState_ = false, isDelayActive_ = false;
    int prevMenuBind_ = -1;
    int widthOld_ = 0, heightOld_ = 0, posXOld_ = 0, posYOld_ = 0;
    DWORD positionFailure_ = ERROR_SUCCESS;
    std::optional<bool> requestedCaptureState_;
    std::array<bool, 256> keyState_ = [] {
        std::array<bool, 256> keys;
        keys.fill(true);
        return keys;
    }();

    template <typename T> inline VOID safeRelease(T*& p) noexcept;

    void limitFps(int fpsLimit);

    bool createDeviceD3D();
    void cleanupDeviceD3D() noexcept;
    void createRenderTarget() noexcept;
    void cleanupRenderTarget() noexcept;
    bool resizeRenderTarget();

    void handler();
    bool updatePosition();
    void render();
    void renderWatermark();
    void menu();

    FLOAT renderText(ImDrawList* drawList, ImFont* font, const std::string& text, const ImVec2& position,
                     const float size, const ImColor& color, const bool centerX, const bool centerY, const bool outline,
                     const bool background);
    void renderCrosshair(const std::optional<crosshair::Settings>& ch, const crosshair::Frame& frame = {},
                         video::Size viewport = {});
    void circularTimer(const ImVec2& pos, const float min, const float max, const float& time, const int pointsCount,
                       const float radius, const float thickness, const bool top, const ImColor& color);
    ID3D11ShaderResourceView* loadPngTexture(unsigned char* iconPng, unsigned int iconPngSize);

    void overlayLoop();
};
