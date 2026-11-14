#include "graphics/overlay.hpp"

#include "core/config.hpp"
#include "core/globals.hpp"
#include "core/logger.hpp"
#include "game/game_window.hpp"
#include "graphics/d3d_resize.hpp"
#include "graphics/image_data.hpp"
#include "platform/input.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <dwmapi.h>
#include <format>
#include <system_error>
#include <thread>

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>

std::atomic<float> Overlay::scrollDelta = 0.f;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void registerRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        Logger::logWarning("Failed to register raw input!");
    }
}

LRESULT WINAPI wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (globals::previewMode && ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;
    switch (msg) {
    case WM_INPUT: {
        RAWINPUT raw{};
        UINT size = sizeof(raw);
        const UINT copied =
            GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
        if (copied == static_cast<UINT>(-1)) {
            OutputDebugStringW(L"ErScripts: cannot read raw mouse input.\n");
            break;
        }
        if (copied >= sizeof(raw) && raw.header.dwType == RIM_TYPEMOUSE &&
            (raw.data.mouse.usButtonFlags & RI_MOUSE_WHEEL))
            Overlay::scrollDelta.fetch_add(static_cast<short>(raw.data.mouse.usButtonData) /
                                           static_cast<float>(WHEEL_DELTA));
        break;
    }
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_CLOSE:
        globals::finish = true;
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

Overlay::~Overlay() {
    globals::finish = true;
    if (renderThread_.joinable())
        renderThread_.join();
}

void Overlay::run() {
    if (!renderThread_.joinable())
        renderThread_ = std::thread(&Overlay::overlayLoop, this);
}

void Overlay::limitFps(int fpsLimit) {
    const auto now = std::chrono::steady_clock::now();
    const auto frame = std::chrono::microseconds(1000000 / std::clamp(fpsLimit, 1, 1000));
    frameDeadline_ = std::max(frameDeadline_ + frame, now);
    std::this_thread::sleep_until(frameDeadline_);
}

bool Overlay::resizeRenderTarget() {
    const auto result =
        graphics::resize(device_, deviceContext_, swapChain_, renderTarget_, globals::width, globals::height);
    if (FAILED(result)) {
        if (result != resizeFailure_)
            Logger::logWarning(std::format("Overlay resize failed: 0x{:08X}", static_cast<unsigned>(result)));
        resizeFailure_ = result;
        if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET ||
            result == DXGI_ERROR_DEVICE_HUNG)
            globals::finish = true;
        return false;
    }
    resizeFailure_ = S_OK;
    if (result == S_OK)
        swapChainOccluded_ = false;
    return true;
}

void Overlay::overlayLoop() {
    ImGui_ImplWin32_EnableDpiAwareness();
    if (!globals::previewMode) {
        int width = 0, height = 0;
        game::windowBounds(width, height, globals::posX, globals::posY);
        if (width > 0 && height > 0) {
            globals::width = width;
            globals::height = height;
        }
    }

    WNDCLASSEXW wc = {sizeof(wc), CS_CLASSDC, wndProc,         0L,     0L, GetModuleHandle(nullptr), nullptr, nullptr,
                      nullptr,    nullptr,    windowClassName, nullptr};
    struct Cleanup {
        Overlay& owner;
        const WNDCLASSEXW& windowClass;
        bool registered = false, win32Ready = false, dx11Ready = false;
        ImGuiContext* context = nullptr;
        ~Cleanup() {
            if (dx11Ready)
                ImGui_ImplDX11_Shutdown();
            if (win32Ready)
                ImGui_ImplWin32_Shutdown();
            if (context)
                ImGui::DestroyContext(context);
            owner.cleanupDeviceD3D();
            if (owner.windowHandle_)
                DestroyWindow(owner.windowHandle_);
            owner.windowHandle_ = nullptr;
            owner.drawList_ = nullptr;
            owner.menuFont_ = owner.arialFont_ = owner.boldFont_ = nullptr;
            if (registered)
                UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
        }
    } cleanup{*this, wc};
    try {
        if (!::RegisterClassEx(&wc))
            throw std::system_error(GetLastError(), std::system_category(), "Cannot register overlay window class");
        cleanup.registered = true;
        windowHandle_ = ::CreateWindow(wc.lpszClassName,
                                       globals::previewMode ? L"ErScripts Preview" : L"GDI+ Window (Lightshot.exe)",
                                       (globals::previewMode ? WS_OVERLAPPEDWINDOW : WS_POPUP), 0, 0, globals::width,
                                       globals::height, nullptr, nullptr, wc.hInstance, nullptr);

        if (!windowHandle_)
            throw std::system_error(GetLastError(), std::system_category(), "Cannot create overlay window");
        if (!globals::previewMode)
            registerRawInput(windowHandle_);
        if (!globals::previewMode)
            SetWindowLongPtr(windowHandle_, GWL_EXSTYLE,
                             WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
        SetLayeredWindowAttributes(windowHandle_, 0, 255, LWA_ALPHA);
        MARGINS margins = {-1, -1, -1, -1};
        DwmExtendFrameIntoClientArea(windowHandle_, &margins);

        if (!createDeviceD3D())
            throw std::runtime_error("Cannot create overlay graphics resources");

        SetWindowPos(windowHandle_, HWND_TOPMOST, globals::posX, globals::posY, globals::width, globals::height,
                     SWP_SHOWWINDOW | SWP_NOACTIVATE);
        ::UpdateWindow(windowHandle_);

        IMGUI_CHECKVERSION();
        cleanup.context = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;

        static constexpr ImWchar smallRange[] = {
            0x0020, 0x007F, 0x0400, 0x04FF, 0x0370, 0x03FF, 0x2000, 0x206F, 0,
        };

        static constexpr ImWchar fullRange[] = {
            0x0020, 0x007F, 0x0400, 0x04FF, 0x0370, 0x03FF, 0x2000, 0x206F, 0x2500, 0x257F, 0x2E80, 0x2EFF,
            0x4E00, 0x9FFF, 0xFF00, 0xFFEF, 0x3000, 0x303F, 0x3130, 0x318F, 0xAC00, 0xD7AF, 0,
        };

        menuFont_ = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.f, nullptr, smallRange);
        if (menuFont_ == nullptr) {
            Logger::logWarning("Failed to load font segoeui.ttf, falling back to default font");
            menuFont_ = io.Fonts->AddFontDefault();
        }

        arialFont_ = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 32.f, nullptr, fullRange);
        if (arialFont_ == nullptr) {
            Logger::logWarning("Failed to load font arial.ttf, falling back to default font");
            arialFont_ = io.Fonts->AddFontDefault();
        }

        boldFont_ = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/ariblk.ttf", 32.f, nullptr, smallRange);
        if (boldFont_ == nullptr) {
            Logger::logWarning("Failed to load font ariblk.ttf, falling back to default font");
            boldFont_ = io.Fonts->AddFontDefault();
        }

        ImGui::StyleColorsDark();

        cleanup.win32Ready = ImGui_ImplWin32_Init(windowHandle_);
        cleanup.dx11Ready = cleanup.win32Ready && ImGui_ImplDX11_Init(device_, deviceContext_);
        if (!cleanup.dx11Ready)
            throw std::runtime_error("Cannot initialize overlay rendering");

        const float clearColor[4]{0};

        ImGuiContext& g = *GImGui;
        g.ConfigNavWindowingKeyNext = 0;
        g.ConfigNavWindowingKeyPrev = 0;

        while (!globals::finish) {

            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT) {
                    globals::finish = true;
                }
            }

            if (globals::finish)
                break;

            if (!globals::previewMode)
                handler();
            else {
                RECT client{};
                GetClientRect(windowHandle_, &client);
                if (client.right > 0 && client.bottom > 0 &&
                    (client.right != globals::width || client.bottom != globals::height)) {
                    globals::width = client.right;
                    globals::height = client.bottom;
                }
            }
            globals::setWindowBounds({globals::width, globals::height, globals::posX, globals::posY});
            if (!resizeRenderTarget()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }
            if (swapChainOccluded_ && swapChain_->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            swapChainOccluded_ = false;

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            if (globals::previewMode && ImGui::IsKeyPressed(ImGuiKey_Insert))
                globals::menuState = !globals::menuState;

            ImGui::Begin(" ", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

            drawList_ = ImGui::GetBackgroundDrawList();

            if (globals::previewMode || game::hasFocus()) {
                if (!globals::previewMode)
                    Overlay::render();
                else
                    renderWatermark();

                if (globals::menuState)
                    Overlay::menu();
            }

            cfg->publish();

            if (cfg->fpsLimiterState && !cfg->vsyncState) {
                limitFps(cfg->fpsLimiter);
            }

            ImGui::End();

            ImGui::Render();
            deviceContext_->OMSetRenderTargets(1, &renderTarget_, nullptr);
            deviceContext_->ClearRenderTargetView(renderTarget_, clearColor);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            HRESULT hr = swapChain_->Present(cfg->vsyncState, 0);
            swapChainOccluded_ = (hr == DXGI_STATUS_OCCLUDED);
            if (FAILED(hr)) {
                Logger::logWarning(std::format("Overlay presentation failed: 0x{:08X}", static_cast<unsigned>(hr)));
                globals::finish = true;
            }
        }

    } catch (const std::exception& error) {
        globals::finish = true;
        Logger::logWarning(std::format("Overlay stopped: {}", error.what()));
    }
    globals::finish = true;
}

std::map<int, std::pair<char, char>> keyMap = {
    {VK_OEM_1, {';', ':'}},     {VK_OEM_PLUS, {'=', '+'}},   {VK_OEM_COMMA, {',', '<'}},
    {VK_OEM_MINUS, {'-', '_'}}, {VK_OEM_PERIOD, {'.', '>'}}, {VK_OEM_2, {'/', '?'}},
    {VK_OEM_3, {'`', '~'}},     {VK_OEM_4, {'[', '{'}},      {VK_OEM_5, {'\\', '|'}},
    {VK_OEM_6, {']', '}'}},     {VK_OEM_7, {'\'', '\"'}},    {'1', {'1', '!'}},
    {'2', {'2', '@'}},          {'3', {'3', '#'}},           {'4', {'4', '$'}},
    {'5', {'5', '%'}},          {'6', {'6', '^'}},           {'7', {'7', '&'}},
    {'8', {'8', '*'}},          {'9', {'9', '('}},           {'0', {'0', ')'}}};

void toggleKey(int vkKey, bool& toggleState, ImGuiIO& io) {

    bool shiftHeld = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000);

    char charCode = 0;

    if (vkKey >= 'A' && vkKey <= 'Z') {
        charCode = shiftHeld ? (char)vkKey : (char)(vkKey + 32);
    } else if (vkKey >= '0' && vkKey <= '9') {
        charCode = shiftHeld ? keyMap[(char)vkKey].second : keyMap[(char)vkKey].first;
    } else if (keyMap.find(vkKey) != keyMap.end()) {

        charCode = shiftHeld ? keyMap[vkKey].second : keyMap[vkKey].first;
    } else if (vkKey == VK_SPACE) {
        charCode = (char)VK_SPACE;
    }

    if (toggleState && (GetAsyncKeyState(vkKey) & 0x8000)) {
        if (charCode != 0)
            io.AddInputCharacter(charCode);
        toggleState = false;
    } else if (!toggleState && !(GetAsyncKeyState(vkKey) & 0x8000)) {
        toggleState = true;
    }
}

void toggleKey(int vkKey, bool& toggleState, ImGuiMouseButton_ imguiKey, ImGuiIO& io) {
    if (toggleState && (GetAsyncKeyState(vkKey) & 0x8000)) {
        io.AddMouseButtonEvent(imguiKey, true);
        toggleState = false;
    } else if (!toggleState && !(GetAsyncKeyState(vkKey) & 0x8000)) {
        io.AddMouseButtonEvent(imguiKey, false);
        toggleState = true;
    }
}

void toggleKey(int vkKey, bool& toggleState, ImGuiKey imguiKey, ImGuiKey imguiModKey, ImGuiIO& io) {
    if (toggleState && (GetAsyncKeyState(vkKey) & 0x8000)) {
        io.AddKeyEvent(imguiKey, true);

        if (imguiModKey) {
            io.AddKeyEvent(imguiModKey, true);
        }
        toggleState = false;
    } else if (!toggleState && !(GetAsyncKeyState(vkKey) & 0x8000)) {
        io.AddKeyEvent(imguiKey, false);

        if (imguiModKey) {
            io.AddKeyEvent(imguiModKey, false);
        }
        toggleState = true;
    }
}

bool Overlay::updatePosition() {
    if (widthOld_ == globals::width && heightOld_ == globals::height && posXOld_ == globals::posX &&
        posYOld_ == globals::posY)
        return true;
    if (!SetWindowPos(windowHandle_, HWND_TOPMOST, globals::posX, globals::posY, globals::width, globals::height,
                      SWP_NOACTIVATE)) {
        const DWORD error = GetLastError();
        if (positionFailure_ != error)
            Logger::logWarning(std::format("Cannot position overlay window: {}", error));
        positionFailure_ = error;
        return false;
    }
    positionFailure_ = ERROR_SUCCESS;
    widthOld_ = globals::width;
    heightOld_ = globals::height;
    posXOld_ = globals::posX;
    posYOld_ = globals::posY;
    return true;
}

void Overlay::handler() {
    if (prevMenuBind_ < 0)
        prevMenuBind_ = cfg->erScriptsMenuBind;

    if (prevMenuBind_ != cfg->erScriptsMenuBind) {
        prevMenuBind_ = cfg->erScriptsMenuBind;
        changeTime_ = std::chrono::steady_clock::now();
        isDelayActive_ = true;
    } else if (isDelayActive_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - changeTime_).count();
        if (elapsed >= 1000) {
            isDelayActive_ = false;
        }
    } else {
        curMenuState_ = (GetAsyncKeyState(cfg->erScriptsMenuBind) & 0x8000) != 0;
    }

    if (curMenuState_ && !prevMenuState_) {
        globals::menuState = !globals::menuState;

        if (globals::menuState && game::hasFocus() && game::cursorHidden()) {
            input::KeyPress key('U');
            showChat_ = key.active();
            if (key.active())
                std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 100 + 16)));
        }
    }
    if (showChat_ && !globals::menuState && game::hasFocus()) {
        input::KeyPress key(VK_ESCAPE);
        if (key.active()) {
            showChat_ = false;
            std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 100 + 16)));
        }
    }

    prevMenuState_ = curMenuState_;

    if (requestedCaptureState_ != cfg->captureBypassState) {
        requestedCaptureState_ = cfg->captureBypassState;
        if (!SetWindowDisplayAffinity(windowHandle_, *requestedCaptureState_ ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE))
            Logger::logWarning(std::format("Cannot set overlay capture exclusion: {}", GetLastError()));
    }

    game::windowBounds(globals::width, globals::height, globals::posX, globals::posY);
    if (globals::width <= 0 || globals::height <= 0) {
        globals::width = widthOld_;
        globals::height = heightOld_;
        return;
    }

    updatePosition();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    POINT cursorPos;
    if (GetCursorPos(&cursorPos)) {
        ScreenToClient(windowHandle_, &cursorPos);
        io.AddMousePosEvent((float)cursorPos.x, (float)cursorPos.y);
    }

    if (const float scroll = scrollDelta.exchange(0.f); scroll != 0.f)
        io.AddMouseWheelEvent(0.0f, scroll);

    for (int vkKey = 'A'; vkKey <= 'Z'; ++vkKey) {
        toggleKey(vkKey, keyState_[vkKey], io);
    }

    for (int vkKey = '0'; vkKey <= '9'; ++vkKey) {
        toggleKey(vkKey, keyState_[vkKey], io);
    }

    toggleKey(VK_LBUTTON, keyState_[VK_LBUTTON], ImGuiMouseButton_Left, io);
    toggleKey(VK_RBUTTON, keyState_[VK_RBUTTON], ImGuiMouseButton_Right, io);
    toggleKey(VK_MBUTTON, keyState_[VK_MBUTTON], ImGuiMouseButton_Middle, io);
    toggleKey(VK_LCONTROL, keyState_[VK_LCONTROL], ImGuiKey_LeftCtrl, ImGuiMod_Ctrl, io);
    toggleKey(VK_LSHIFT, keyState_[VK_LSHIFT], ImGuiKey_LeftShift, ImGuiMod_Shift, io);
    toggleKey(VK_BACK, keyState_[VK_BACK], ImGuiKey_Backspace, ImGuiMod_None, io);
    toggleKey(VK_RETURN, keyState_[VK_RETURN], ImGuiKey_Enter, ImGuiMod_None, io);
    toggleKey(VK_TAB, keyState_[VK_TAB], ImGuiKey_Tab, ImGuiMod_None, io);
    toggleKey(VK_DELETE, keyState_[VK_DELETE], ImGuiKey_Delete, ImGuiMod_None, io);
    toggleKey(VK_UP, keyState_[VK_UP], ImGuiKey_UpArrow, ImGuiMod_None, io);
    toggleKey(VK_DOWN, keyState_[VK_DOWN], ImGuiKey_DownArrow, ImGuiMod_None, io);
    toggleKey(VK_LEFT, keyState_[VK_LEFT], ImGuiKey_LeftArrow, ImGuiMod_None, io);
    toggleKey(VK_RIGHT, keyState_[VK_RIGHT], ImGuiKey_RightArrow, ImGuiMod_None, io);

    toggleKey(VK_OEM_1, keyState_[VK_OEM_1], io);
    toggleKey(VK_OEM_PLUS, keyState_[VK_OEM_PLUS], io);
    toggleKey(VK_OEM_COMMA, keyState_[VK_OEM_COMMA], io);
    toggleKey(VK_OEM_MINUS, keyState_[VK_OEM_MINUS], io);
    toggleKey(VK_OEM_PERIOD, keyState_[VK_OEM_PERIOD], io);
    toggleKey(VK_SPACE, keyState_[VK_SPACE], io);
    toggleKey(VK_OEM_2, keyState_[VK_OEM_2], io);
    toggleKey(VK_OEM_3, keyState_[VK_OEM_3], io);
    toggleKey(VK_OEM_4, keyState_[VK_OEM_4], io);
    toggleKey(VK_OEM_5, keyState_[VK_OEM_5], io);
    toggleKey(VK_OEM_6, keyState_[VK_OEM_6], io);
    toggleKey(VK_OEM_7, keyState_[VK_OEM_7], io);
}

bool Overlay::createDeviceD3D() {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = windowHandle_;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res =
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2,
                                      D3D11_SDK_VERSION, &sd, &swapChain_, &device_, &featureLevel, &deviceContext_);
    if (res == DXGI_ERROR_UNSUPPORTED) {
        Logger::logWarning("Render Type Changed to WARP");
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, featureLevelArray, 2,
                                            D3D11_SDK_VERSION, &sd, &swapChain_, &device_, &featureLevel,
                                            &deviceContext_);
    }
    if (res != S_OK)
        return false;

    createRenderTarget();
    if (!renderTarget_)
        return false;

    bombTexture_ = loadPngTexture(c4Icon, c4IconLen);

    return true;
}

void Overlay::cleanupDeviceD3D() noexcept {
    cleanupRenderTarget();

    safeRelease(swapChain_);
    safeRelease(deviceContext_);
    safeRelease(device_);
    safeRelease(bombTexture_);
}

void Overlay::createRenderTarget() noexcept {
    ID3D11Texture2D* pBackBuffer = nullptr;
    swapChain_->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
        device_->CreateRenderTargetView(pBackBuffer, nullptr, &renderTarget_);

    if (pBackBuffer)
        pBackBuffer->Release();
}

void Overlay::cleanupRenderTarget() noexcept {
    safeRelease(renderTarget_);
}

template <typename T> inline VOID Overlay::safeRelease(T*& p) noexcept {
    if (nullptr != p) {
        p->Release();
        p = nullptr;
    }
}
