#include "overlay.h"
#include "D3DResize.h"
#include <thread>

std::atomic<float> Overlay::g_ScrollDelta = 0.f;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void RegisterRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        Logger::logWarning("Failed to register raw input!");
    }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (globals::previewMode && ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return 1;
    switch (msg) {
    case WM_INPUT: {
        UINT dwSize = 0;

        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER)) ==
            -1) {
            Logger::logWarning("GetRawInputData failed to get buffer size!");
            break;
        }

        std::vector<BYTE> inputData(dwSize);

        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, inputData.data(), &dwSize,
                            sizeof(RAWINPUTHEADER)) == -1) {
            Logger::logWarning("GetRawInputData failed to get input data!");
            break;
        }

        RAWINPUT *rawInput = reinterpret_cast<RAWINPUT *>(inputData.data());

        if (rawInput->header.dwType == RIM_TYPEMOUSE) {
            if (rawInput->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                Overlay::g_ScrollDelta = (static_cast<short>(rawInput->data.mouse.usButtonData) > 0) ? 1.0f : -1.0f;
            }
        }
        break;
    }
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

Overlay::~Overlay() {
    globals::finish = true;
    if (renderThread.joinable())
        renderThread.join();
}

void Overlay::run() {
    if (!renderThread.joinable())
        renderThread = std::thread(&Overlay::OverlayLoop, this);
}

void Overlay::LimitFPS(int fpsLimit) noexcept {
    static auto deadline = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    const auto frame = std::chrono::microseconds(1000000 / std::clamp(fpsLimit, 1, 1000));
    deadline = std::max(deadline + frame, now);
    std::this_thread::sleep_until(deadline);
}

bool Overlay::ResizeRenderTarget() noexcept {
    const auto result = graphics::resize(g_pd3dDevice, g_pd3dDeviceContext, g_pSwapChain, g_mainRenderTargetView,
                                         globals::width, globals::height);
    if (FAILED(result)) {
        if (result != resizeFailure)
            Logger::logWarning(std::format("Overlay resize failed: 0x{:08X}", static_cast<unsigned>(result)));
        resizeFailure = result;
        return false;
    }
    resizeFailure = S_OK;
    if (result == S_OK)
        g_SwapChainOccluded = false;
    return true;
}

void Overlay::OverlayLoop() noexcept {
    ImGui_ImplWin32_EnableDpiAwareness();
    if (!globals::previewMode) {
        int width = 0, height = 0;
        ErScripts::GetWindowInfo(width, height, globals::posX, globals::posY);
        if (width > 0 && height > 0) {
            globals::width = width;
            globals::height = height;
        }
    }

    WNDCLASSEXW wc = {sizeof(wc),
                      CS_CLASSDC,
                      WndProc,
                      0L,
                      0L,
                      GetModuleHandle(nullptr),
                      nullptr,
                      nullptr,
                      nullptr,
                      nullptr,
                      L"GDI+ Hook Window Class",
                      nullptr};
    ::RegisterClassEx(&wc);
    window_handle =
        ::CreateWindow(wc.lpszClassName, globals::previewMode ? L"ErScripts Preview" : L"GDI+ Window (Lightshot.exe)",
                       (globals::previewMode ? WS_OVERLAPPEDWINDOW : WS_POPUP), 0, 0, globals::width, globals::height,
                       nullptr, nullptr, wc.hInstance, nullptr);

    if (!globals::previewMode)
        SetWindowLongPtr(window_handle, GWL_EXSTYLE,
                         WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
    SetLayeredWindowAttributes(window_handle, 0, 255, LWA_ALPHA);
    MARGINS margins = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(window_handle, &margins);

    if (!CreateDeviceD3D()) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        globals::finish = true;
        return;
    }

    SetWindowPos(window_handle, HWND_TOPMOST, globals::posX, globals::posY, globals::width, globals::height,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE);
    ::UpdateWindow(window_handle);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.IniFilename = NULL;
    io.LogFilename = NULL;

    const ImWchar small_range[] = {
        0x0020, 0x007F, 0x0400, 0x04FF, 0x0370, 0x03FF, 0x2000, 0x206F, 0,
    };

    const ImWchar full_range[] = {
        0x0020, 0x007F, 0x0400, 0x04FF, 0x0370, 0x03FF, 0x2000, 0x206F, 0x2500, 0x257F, 0x2E80, 0x2EFF,
        0x4E00, 0x9FFF, 0xFF00, 0xFFEF, 0x3000, 0x303F, 0x3130, 0x318F, 0xAC00, 0xD7AF, 0,
    };

    menu_font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.f, nullptr, small_range);
    if (menu_font == nullptr) {
        Logger::logWarning("Failed to load font arial.ttf, falling back to default font");
        menu_font = io.Fonts->AddFontDefault();
    }

    arial_font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 32.f, nullptr, full_range);
    if (arial_font == nullptr) {
        Logger::logWarning("Failed to load font arial.ttf, falling back to default font");
        arial_font = io.Fonts->AddFontDefault();
    }

    bold_font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/ariblk.ttf", 32.f, nullptr, small_range);
    if (bold_font == nullptr) {
        Logger::logWarning("Failed to load font ariblk.ttf, falling back to default font");
        bold_font = io.Fonts->AddFontDefault();
    }

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window_handle);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    const float clear_color[4]{0};

    ImGuiContext &g = *GImGui;
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

        if (!isFinish)
            globals::finish = true;

        if (globals::finish)
            break;

        if (!globals::previewMode)
            Handler();
        else {
            RECT client{};
            GetClientRect(window_handle, &client);
            if (client.right > 0 && client.bottom > 0 &&
                (client.right != globals::width || client.bottom != globals::height)) {
                globals::width = client.right;
                globals::height = client.bottom;
            }
        }
        if (!ResizeRenderTarget()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        g_SwapChainOccluded = false;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (globals::previewMode && ImGui::IsKeyPressed(ImGuiKey_Insert))
            globals::menuState = !globals::menuState;

        ImGui::Begin(" ", 0,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

        draw_list = ImGui::GetBackgroundDrawList();

        if (globals::previewMode || ErScripts::GetWindowState()) {
            if (!globals::previewMode)
                Overlay::Render();
            else
                RenderWatermark();

            if (globals::menuState)
                Overlay::Menu();
        }

        if (cfg->fpsLimiterState && !cfg->vsyncState) {
            LimitFPS(cfg->fpsLimiter);
        }

        ImGui::End();

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(cfg->vsyncState, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(window_handle);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

std::map<int, std::pair<char, char>> keyMap = {
    {VK_OEM_1, {';', ':'}},     {VK_OEM_PLUS, {'=', '+'}},   {VK_OEM_COMMA, {',', '<'}},
    {VK_OEM_MINUS, {'-', '_'}}, {VK_OEM_PERIOD, {'.', '>'}}, {VK_OEM_2, {'/', '?'}},
    {VK_OEM_3, {'`', '~'}},     {VK_OEM_4, {'[', '{'}},      {VK_OEM_5, {'\\', '|'}},
    {VK_OEM_6, {']', '}'}},     {VK_OEM_7, {'\'', '\"'}},    {'1', {'1', '!'}},
    {'2', {'2', '@'}},          {'3', {'3', '#'}},           {'4', {'4', '$'}},
    {'5', {'5', '%'}},          {'6', {'6', '^'}},           {'7', {'7', '&'}},
    {'8', {'8', '*'}},          {'9', {'9', '('}},           {'0', {'0', ')'}}};

void toggleKey(int vkKey, bool &toggleState, ImGuiIO &io) {

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

void toggleKey(int vkKey, bool &toggleState, ImGuiMouseButton_ imguiKey, ImGuiIO &io) {
    if (toggleState && (GetAsyncKeyState(vkKey) & 0x8000)) {
        io.AddMouseButtonEvent(imguiKey, true);
        toggleState = false;
    } else if (!toggleState && !(GetAsyncKeyState(vkKey) & 0x8000)) {
        io.AddMouseButtonEvent(imguiKey, false);
        toggleState = true;
    }
}

void toggleKey(int vkKey, bool &toggleState, ImGuiKey imguiKey, ImGuiKey imguiModKey, ImGuiIO &io) {
    if (toggleState && (GetAsyncKeyState(vkKey) & 0x8000)) {
        io.AddKeyEvent(imguiKey, true);

        if (imguiModKey) {
            ImGui::GetKeyData(imguiModKey)->Down = true;
        }
        toggleState = false;
    } else if (!toggleState && !(GetAsyncKeyState(vkKey) & 0x8000)) {
        io.AddKeyEvent(imguiKey, false);

        if (imguiModKey) {
            ImGui::GetKeyData(imguiModKey)->Down = false;
        }
        toggleState = true;
    }
}

void Overlay::Handler() noexcept {
    static bool showChat = false;
    static bool prevMenuState = false;
    static bool inCaptureBypass = !cfg->captureBypassState;
    static int width_old, height_old, posX_old, posY_old;

    static bool curMenuState = (GetAsyncKeyState(cfg->erScriptsMenuBind) & 0x8000) != 0;
    static int prevMenuBind = cfg->erScriptsMenuBind;
    static std::chrono::steady_clock::time_point changeTime;
    static bool isDelayActive = false;

    static bool initialized = false;
    if (!initialized) {
        RegisterRawInput(window_handle);

        initialized = true;
    }

    if (prevMenuBind != cfg->erScriptsMenuBind) {
        prevMenuBind = cfg->erScriptsMenuBind;
        changeTime = std::chrono::steady_clock::now();
        isDelayActive = true;
    } else if (isDelayActive) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - changeTime).count();
        if (elapsed >= 1000) {
            isDelayActive = false;
        }
    } else {
        curMenuState = (GetAsyncKeyState(cfg->erScriptsMenuBind) & 0x8000) != 0;
    }

    if (curMenuState && !prevMenuState) {
        globals::menuState = !globals::menuState;

        if (globals::menuState && ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
            showChat = true;
            ErScripts::Keyboard(0x55, true);
            std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 100 + 16)));
            ErScripts::Keyboard(0x55, false);
        } else if (showChat) {
            showChat = false;
            ErScripts::Keyboard(VK_ESCAPE, true);
            std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 100 + 16)));
            ErScripts::Keyboard(VK_ESCAPE, false);
        }
    }

    prevMenuState = curMenuState;

    if (inCaptureBypass != cfg->captureBypassState) {
        inCaptureBypass = cfg->captureBypassState;
        SetWindowDisplayAffinity(window_handle, inCaptureBypass ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
    }

    ErScripts::GetWindowInfo(globals::width, globals::height, globals::posX, globals::posY);
    if (globals::width <= 0 || globals::height <= 0) {
        globals::width = width_old;
        globals::height = height_old;
        return;
    }

    if ((width_old != globals::width || height_old != globals::height) ||
        (posX_old != globals::posX || posY_old != globals::posY)) {
        width_old = globals::width;
        height_old = globals::height;
        posX_old = globals::posX;
        posY_old = globals::posY;
        SetWindowPos(window_handle, HWND_TOPMOST, globals::posX, globals::posY, globals::width, globals::height,
                     SWP_NOACTIVATE);
    }

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;

    POINT cursorPos;
    if (GetCursorPos(&cursorPos)) {
        ScreenToClient(window_handle, &cursorPos);
        io.AddMousePosEvent((float)cursorPos.x, (float)cursorPos.y);
    }

    if (g_ScrollDelta) {
        io.AddMouseWheelEvent(0.0f, g_ScrollDelta);
        g_ScrollDelta = 0.0f;
    }

    static bool keyState[256] = {true};

    for (int vkKey = 'A'; vkKey <= 'Z'; ++vkKey) {
        toggleKey(vkKey, keyState[vkKey], io);
    }

    for (int vkKey = '0'; vkKey <= '9'; ++vkKey) {
        toggleKey(vkKey, keyState[vkKey], io);
    }

    static bool LMouseState = true, RMouseState = true, MMouseState = true;
    static bool LeftCtrl = true, LeftShift = true, Backspace = true, Enter = true, Tab = true, Delete = true,
                ArrowUp = true, ArrowDown = true, ArrowLeft = true, ArrowRight = true;

    toggleKey(VK_LBUTTON, LMouseState, ImGuiMouseButton_Left, io);
    toggleKey(VK_RBUTTON, RMouseState, ImGuiMouseButton_Right, io);
    toggleKey(VK_MBUTTON, MMouseState, ImGuiMouseButton_Middle, io);
    toggleKey(VK_LCONTROL, LeftCtrl, ImGuiKey_LeftCtrl, ImGuiMod_Ctrl, io);
    toggleKey(VK_LSHIFT, LeftShift, ImGuiKey_LeftShift, ImGuiMod_Shift, io);
    toggleKey(VK_BACK, Backspace, ImGuiKey_Backspace, ImGuiMod_None, io);
    toggleKey(VK_RETURN, Enter, ImGuiKey_Enter, ImGuiMod_None, io);
    toggleKey(VK_TAB, Tab, ImGuiKey_Tab, ImGuiMod_None, io);
    toggleKey(VK_DELETE, Delete, ImGuiKey_Delete, ImGuiMod_None, io);
    toggleKey(VK_UP, ArrowUp, ImGuiKey_UpArrow, ImGuiMod_None, io);
    toggleKey(VK_DOWN, ArrowDown, ImGuiKey_DownArrow, ImGuiMod_None, io);
    toggleKey(VK_LEFT, ArrowLeft, ImGuiKey_LeftArrow, ImGuiMod_None, io);
    toggleKey(VK_RIGHT, ArrowRight, ImGuiKey_RightArrow, ImGuiMod_None, io);

    toggleKey(VK_OEM_1, keyState[VK_OEM_1], io);
    toggleKey(VK_OEM_PLUS, keyState[VK_OEM_PLUS], io);
    toggleKey(VK_OEM_COMMA, keyState[VK_OEM_COMMA], io);
    toggleKey(VK_OEM_MINUS, keyState[VK_OEM_MINUS], io);
    toggleKey(VK_OEM_PERIOD, keyState[VK_OEM_PERIOD], io);
    toggleKey(VK_SPACE, keyState[VK_SPACE], io);
    toggleKey(VK_OEM_2, keyState[VK_OEM_2], io);
    toggleKey(VK_OEM_3, keyState[VK_OEM_3], io);
    toggleKey(VK_OEM_4, keyState[VK_OEM_4], io);
    toggleKey(VK_OEM_5, keyState[VK_OEM_5], io);
    toggleKey(VK_OEM_6, keyState[VK_OEM_6], io);
    toggleKey(VK_OEM_7, keyState[VK_OEM_7], io);
}

bool Overlay::CreateDeviceD3D() noexcept {
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
    sd.OutputWindow = window_handle;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2,
                                                D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel,
                                                &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) {
        Logger::logWarning("Render Type Changed to WARP");
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, featureLevelArray, 2,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel,
                                            &g_pd3dDeviceContext);
    }
    if (res != S_OK)
        return false;

    CreateRenderTarget();

    bombTexture = LoadTextureFromPNGShellcode(c4Icon, c4IconLen);
    settingsTexture = LoadTextureFromPNGShellcode(settingsIcon, settingsIconLen);
    reloadTexture = LoadTextureFromPNGShellcode(reloadIcon, reloadIconLen);

    return true;
}

void Overlay::CleanupDeviceD3D() noexcept {
    CleanupRenderTarget();

    SafeRelease(g_pSwapChain);
    SafeRelease(g_pd3dDeviceContext);
    SafeRelease(g_pd3dDevice);
    SafeRelease(bombTexture);
    SafeRelease(settingsTexture);
    SafeRelease(reloadTexture);
}

void Overlay::CreateRenderTarget() noexcept {
    ID3D11Texture2D *pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer)
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);

    if (pBackBuffer)
        pBackBuffer->Release();
}

void Overlay::CleanupRenderTarget() noexcept {
    SafeRelease(g_mainRenderTargetView);
}

template <typename T> inline VOID Overlay::SafeRelease(T *&p) noexcept {
    if (nullptr != p) {
        p->Release();
        p = nullptr;
    }
}
