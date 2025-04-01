#include "overlay.h"
#include <thread>

void Overlay::run() {
	std::thread serverThread(&Overlay::OverlayLoop, this);
	serverThread.detach();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline float g_ScrollDelta = 0.0f;

// Register Raw Input to capture mouse scroll events globally
void RegisterRawInput(HWND hwnd) {
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;			// Generic Desktop Controls
	rid.usUsage = 0x02;				// Mouse
	rid.dwFlags = RIDEV_INPUTSINK;	// Capture input even if the window is unfocused
	rid.hwndTarget = hwnd;

	if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
		Logger::logWarning("Failed to register raw input!");
	}
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_INPUT: {
		UINT dwSize = 0;

		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER)) == -1) {
			Logger::logWarning("GetRawInputData failed to get buffer size!");
			break;
		}

		std::vector<BYTE> inputData(dwSize);

		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, inputData.data(), &dwSize, sizeof(RAWINPUTHEADER)) == -1) {
			Logger::logWarning("GetRawInputData failed to get input data!");
			break;
		}

		RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(inputData.data());

		if (rawInput->header.dwType == RIM_TYPEMOUSE) {
			if (rawInput->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
				g_ScrollDelta = (static_cast<short>(rawInput->data.mouse.usButtonData) > 0) ? 1.0f : -1.0f;
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

void LimitFPS(int fpsLimit) {
	float fps = 1000000.0f / static_cast<float>(fpsLimit);
	static auto awake_time = std::chrono::steady_clock::now() + std::chrono::microseconds(static_cast<int>(fps));
	awake_time += std::chrono::microseconds(static_cast<int>(fps));
	std::this_thread::sleep_until(awake_time);
}

void Overlay::OverlayLoop() noexcept {
	// Create application window
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandleW(nullptr), nullptr, nullptr, nullptr, nullptr, L"GDI+ Hook Window Class", nullptr };
	::RegisterClassExW(&wc);
	window_handle = ::CreateWindowW(wc.lpszClassName, L"GDI+ Window (Lightshot.exe)", WS_POPUP, 0, 0, globals::width, globals::height, nullptr, nullptr, wc.hInstance, nullptr);

	// Initialize Direct3D
	if (!CreateDeviceD3D()) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		globals::finish = true;
		return;
	}

	// Show the window
	::ShowWindow(window_handle, SW_SHOW);
	::UpdateWindow(window_handle);

	MARGINS margins = { -1, -1, -1, -1 };
	DwmExtendFrameIntoClientArea(window_handle, &margins);
	SetWindowLongPtrW(window_handle, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
	SetLayeredWindowAttributes(window_handle, 0, 255, LWA_ALPHA);
	//SetWindowPos(window_handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	io.LogFilename = NULL;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	const ImWchar small_range[] = {
		0x0020, 0x007F,    // Basic Latin (Standard ASCII characters)
		0x0400, 0x04FF,    // Cyrillic
		0x0370, 0x03FF,    // Greek and Coptic
		0x2000, 0x206F,    // General punctuation, currency symbols, etc.
		0,                 // Null-terminate the ranges
	};

	const ImWchar full_range[] = {
		0x0020, 0x007F,    // Basic Latin (Standard ASCII characters)
		0x0400, 0x04FF,    // Cyrillic
		0x0370, 0x03FF,    // Greek and Coptic
		0x2000, 0x206F,    // General punctuation, currency symbols, etc.
		0x2500, 0x257F,    // Box-drawing characters
		0x2E80, 0x2EFF,    // CJK Radicals Supplement
		0x4E00, 0x9FFF,    // CJK Ideographs (Chinese/Japanese/Korean)
		0xFF00, 0xFFEF,    // Half-width and Full-width Forms (including Katakana, Hangul)
		0x3000, 0x303F,    // CJK Symbols and Punctuation
		0x3130, 0x318F,    // Hangul Compatibility Jamo
		0xAC00, 0xD7AF,    // Hangul Syllables (Korean)
		0,                 // Null-terminate the ranges
	};

	menu_font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 16.f, nullptr, small_range);
	arial_font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 32.f, nullptr, full_range);
	bold_font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/ariblk.ttf", 32.f, nullptr, small_range);
	//weapon_font = io.Fonts->AddFontFromMemoryTTF(&shell_weapon_font, sizeof(shell_weapon_font), 32.f);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(window_handle);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	ErScripts::GetWindowInfo(globals::width, globals::height, globals::posX, globals::posY);

	CleanupRenderTarget();
	SetWindowPos(window_handle, (HWND)-1, globals::posX, globals::posY, globals::width, globals::height, 0);
	g_pSwapChain->ResizeBuffers(0, globals::width, globals::height, DXGI_FORMAT_UNKNOWN, 0);
	CreateRenderTarget();

	std::thread(&Overlay::Handler, this).detach();

	const float clear_color[4]{ 0 };
	//UINT prevWidth = globals::width, prevHeight = globals::height;

	// Main loop
	while (!globals::finish) {
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				globals::finish = true;
			}
		}

		if (!isFinish) globals::finish = true;

		if (globals::finish)
			break;

		// Handle window being minimized or screen locked
		if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}
		g_SwapChainOccluded = false;

		/*if (prevWidth != globals::width || prevHeight != globals::height) {
			CleanupRenderTarget();
			SetWindowPos(window_handle, HWND_TOPMOST, 0, 0, globals::width, globals::height, SWP_NOMOVE | SWP_NOSIZE);
			g_pSwapChain->ResizeBuffers(0, globals::width, globals::height, DXGI_FORMAT_UNKNOWN, 0);

			prevWidth = globals::width;
			prevHeight = globals::height;

			CreateRenderTarget();
		}*/

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Create invisible window so debug window won't appear
		ImGui::Begin(" ", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

		// Access the draw list
		draw_list = ImGui::GetBackgroundDrawList();

		if (ErScripts::GetWindowState()) {
			Overlay::Render();

			/*static bool prevMenuState = false;
			if (globals::menuState != prevMenuState) {
				prevMenuState = globals::menuState;

				if (globals::menuState) {
					SetWindowLongPtrW(window_handle, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
				}
				else {
					SetWindowLongPtrW(window_handle, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
				}
			}*/

			if (globals::menuState) Overlay::Menu();
		}

		if (cfg->fpsLimiterState && !cfg->vsyncState) {
			LimitFPS(cfg->fpsLimiter);
		}

		ImGui::End();

		ImGui::Render();
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present
		HRESULT hr = g_pSwapChain->Present(/*cfg->vsyncState*/ 0, 0);
		g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	//ImGui::DestroyContext(); // shutdown in DestroyContext crashing code


	CleanupDeviceD3D();
	::DestroyWindow(window_handle);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

// Map for handling symbol inputs based on Shift state
std::map<int, std::pair<char, char>> keyMap = {
	{VK_OEM_1, {';', ':'}},     // Semicolon
	{VK_OEM_PLUS, {'=', '+'}},   // Equals
	{VK_OEM_COMMA, {',', '<'}},   // Comma
	{VK_OEM_MINUS, {'-', '_'}},   // Minus
	{VK_OEM_PERIOD, {'.', '>'}},   // Period
	{VK_OEM_2, {'/', '?'}},       // Slash
	{VK_OEM_3, {'`', '~'}},       // Backtick
	{VK_OEM_4, {'[', '{'}},   // Left bracket
	{VK_OEM_5, {'\\', '|'}},    // Backslash
	{VK_OEM_6, {']', '}'}}, // Right bracket
	{VK_OEM_7, {'\'', '\"'}},       // Quote
	{'1', {'1', '!'}},            // 1
	{'2', {'2', '@'}},            // 2
	{'3', {'3', '#'}},            // 3
	{'4', {'4', '$'}},            // 4
	{'5', {'5', '%'}},            // 5
	{'6', {'6', '^'}},            // 6
	{'7', {'7', '&'}},            // 7
	{'8', {'8', '*'}},            // 8
	{'9', {'9', '('}},            // 9
	{'0', {'0', ')'}}             // 0
};

void toggleKey(int vkKey, bool& toggleState, ImGuiIO& io) {
	// Check the state of the Shift key
	bool shiftHeld = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_RSHIFT) & 0x8000);

	char charCode = 0; // Initialize character code to null

	// Determine the character based on vkKey and shift state
	if (vkKey >= 'A' && vkKey <= 'Z') {
		charCode = shiftHeld ? (char)vkKey : (char)(vkKey + 32); // Handle A-Z
	}
	else if (vkKey >= '0' && vkKey <= '9') {
		charCode = shiftHeld ? keyMap[(char)vkKey].second : keyMap[(char)vkKey].first; // Handle 0-9
	}
	else if (keyMap.find(vkKey) != keyMap.end()) {
		// Handle special symbols
		charCode = shiftHeld ? keyMap[vkKey].second : keyMap[vkKey].first;
	}
	else if (vkKey == VK_SPACE) {
		charCode = (char)VK_SPACE;
	}

	// Toggle input character based on key state
	if (toggleState && (GetAsyncKeyState(vkKey) & 0x8000)) {
		if (charCode != 0) // Only add input if charCode is valid
			io.AddInputCharacter(charCode);
		toggleState = false; // Set toggle to false to avoid spamming
	}
	else if (!toggleState && !(GetAsyncKeyState(vkKey) & 0x8000)) {
		toggleState = true; // Reset toggle state
	}
}

void toggleKey(int vkKey, bool& toggleState, ImGuiMouseButton_ imguiKey, ImGuiIO& io) {
	if (toggleState && (GetAsyncKeyState(vkKey) & 0x8000)) {
		io.AddMouseButtonEvent(imguiKey, true);
		toggleState = false;
	}
	else if (!toggleState && !(GetAsyncKeyState(vkKey) & 0x8000)) {
		io.AddMouseButtonEvent(imguiKey, false);
		toggleState = true;
	}
}

void toggleKey(int vkKey, bool& toggleState, ImGuiKey imguiKey, ImGuiKey imguiModKey, ImGuiIO& io) {
	if (toggleState && (GetAsyncKeyState(vkKey) & 0x8000)) {
		io.AddKeyEvent(imguiKey, true);

		if (imguiModKey) {
			ImGui::GetKeyData(imguiModKey)->Down = true; // With this working custom number input
		}
		toggleState = false;
	}
	else if (!toggleState && !(GetAsyncKeyState(vkKey) & 0x8000)) {
		io.AddKeyEvent(imguiKey, false);

		if (imguiModKey) {
			ImGui::GetKeyData(imguiModKey)->Down = false;
		}
		toggleState = true;
	}
}

void Overlay::Handler() noexcept {
	bool showChat = false;
	bool prevMenuState = false;
	bool inCaptureBypass = !cfg->captureBypassState;
	int width_old, height_old, posX_old, posY_old;

	bool curMenuState = (GetAsyncKeyState(cfg->erScriptsMenuBind) & 0x8000) != 0;
	int prevMenuBind = cfg->erScriptsMenuBind;
	std::chrono::steady_clock::time_point changeTime;
	bool isDelayActive = false;

	ErScripts::GetWindowInfo(width_old, height_old, posX_old, posY_old);

	CleanupRenderTarget();
	SetWindowPos(window_handle, (HWND)-1, posX_old, posY_old, globals::width, globals::height, 0);
	g_pSwapChain->ResizeBuffers(0, globals::width, globals::height, DXGI_FORMAT_UNKNOWN, 0);
	CreateRenderTarget();

	RegisterRawInput(window_handle);

	while (!globals::finish) {
		if (prevMenuBind != cfg->erScriptsMenuBind) {
			prevMenuBind = cfg->erScriptsMenuBind;
			changeTime = std::chrono::steady_clock::now();
			isDelayActive = true;
		}
		else if (isDelayActive) {
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - changeTime).count();
			if (elapsed >= 1000) {
				isDelayActive = false;
			}
		}
		else {
			curMenuState = (GetAsyncKeyState(cfg->erScriptsMenuBind) & 0x8000) != 0;
		}

		// Check if key state changed from not pressed to pressed
		if (curMenuState && !prevMenuState) {
			globals::menuState = !globals::menuState;

			if (globals::menuState && ErScripts::GetWindowState() && ErScripts::GetCursorState()) {
				showChat = true;
				ErScripts::Keyboard(0x55, true, false); // U
				std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 100 + 16)));
				ErScripts::Keyboard(0x55, false, false);
			}
			else if (showChat) {
				showChat = false;
				ErScripts::Keyboard(VK_ESCAPE, true, false); // ESCAPE
				std::this_thread::sleep_for(std::chrono::milliseconds((std::rand() % 100 + 16)));
				ErScripts::Keyboard(VK_ESCAPE, false, false);
			}
		}

		prevMenuState = curMenuState;

		// Toggle capture bypass
		if (inCaptureBypass != cfg->captureBypassState) {
			inCaptureBypass = cfg->captureBypassState;
			SetWindowDisplayAffinity(window_handle, inCaptureBypass ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
		}

		// Resolution and position changer
		ErScripts::GetWindowInfo(globals::width, globals::height, globals::posX, globals::posY);
		
		if ((width_old != globals::width || height_old != globals::height) || (posX_old != globals::posX || posY_old != globals::posY)) {
			width_old = globals::width;
			height_old = globals::height;
			posX_old = globals::posX;
			posY_old = globals::posY;
			CleanupRenderTarget();
			SetWindowPos(window_handle, (HWND)-1, globals::posX, globals::posY, globals::width, globals::height, 0);
			g_pSwapChain->ResizeBuffers(0, globals::width, globals::height, DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}

		// Input handler
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = NULL;
		io.LogFilename = NULL;

		// Update mouse position
		POINT cursorPos;
		if (GetCursorPos(&cursorPos)) {
			ScreenToClient(window_handle, &cursorPos);
			io.AddMousePosEvent((float)cursorPos.x, (float)cursorPos.y);
		}

		// Update mouse scroll
		if (g_ScrollDelta) {
			io.AddMouseWheelEvent(0.0f, g_ScrollDelta);
			g_ScrollDelta = 0.0f;
		}

		// Alphanumeric characters
		static bool keyState[256] = { true }; // Track each key's state

		// Handle letters A-Z
		for (int vkKey = 'A'; vkKey <= 'Z'; ++vkKey) {
			toggleKey(vkKey, keyState[vkKey], io);
		}

		// Handle numbers 0-9
		for (int vkKey = '0'; vkKey <= '9'; ++vkKey) {
			toggleKey(vkKey, keyState[vkKey], io); // 0-9
		}

		// Special Keys
		static bool LMouseState = true, RMouseState = true, MMouseState = true;
		static bool LeftCtrl = true, LeftShift = true, Backspace = true, Enter = true, Tab = true, Delete = true, ArrowUp = true, ArrowDown = true, ArrowLeft = true, ArrowRight = true;

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

		toggleKey(VK_OEM_1, keyState[VK_OEM_1], io);			// ;
		toggleKey(VK_OEM_PLUS, keyState[VK_OEM_PLUS], io);		// =
		toggleKey(VK_OEM_COMMA, keyState[VK_OEM_COMMA], io);	// <
		toggleKey(VK_OEM_MINUS, keyState[VK_OEM_MINUS], io);	// -
		toggleKey(VK_OEM_PERIOD, keyState[VK_OEM_PERIOD], io);	// >
		toggleKey(VK_SPACE, keyState[VK_SPACE], io);			// Space
		toggleKey(VK_OEM_2, keyState[VK_OEM_2], io);			// /
		toggleKey(VK_OEM_3, keyState[VK_OEM_3], io);			// `
		toggleKey(VK_OEM_4, keyState[VK_OEM_4], io);			// [
		toggleKey(VK_OEM_5, keyState[VK_OEM_5], io);			// |
		toggleKey(VK_OEM_6, keyState[VK_OEM_6], io);			// ]
		toggleKey(VK_OEM_7, keyState[VK_OEM_7], io);			// /

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
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

	//UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, /*createDeviceFlags*/ 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) { // Try high-performance WARP software driver if hardware is not available.
		Logger::logWarning("Render Type Changed to WARP");
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, /*createDeviceFlags*/ 0, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
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
	CleanupRenderTarget(); // Clean up render target

	// Release DirectX COM objects
	SafeRelease(g_pSwapChain);
	SafeRelease(g_pd3dDeviceContext);
	SafeRelease(g_pd3dDevice);
	SafeRelease(bombTexture);
	SafeRelease(settingsTexture);
	SafeRelease(reloadTexture);

	// Clean up dynamically allocated fonts
	delete menu_font;
	delete arial_font;
	delete bold_font;
	//delete weapon_font;
}

void Overlay::CreateRenderTarget() noexcept {
	ID3D11Texture2D* pBackBuffer = nullptr;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (pBackBuffer)
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);

	pBackBuffer->Release();
}

void Overlay::CleanupRenderTarget() noexcept {
	SafeRelease(g_mainRenderTargetView);
}

template <typename T>
inline VOID Overlay::SafeRelease(T*& p) noexcept {
	if (nullptr != p) {
		p->Release();
		p = nullptr;
	}
}