#pragma once

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"dwmapi.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h" // for ImGui::GetKeyData()
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "ErScripts.h"
#include "ImagesShellCodes.h"

#include <dwmapi.h>
#include <d3d11.h>
#include <string>
#include <wincodec.h>
#include <comdef.h>
#include <Windows.h>

class Overlay {
public:
	void run();
private:
	HWND window_handle = nullptr;
	ID3D11Device* g_pd3dDevice = nullptr;
	ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
	IDXGISwapChain* g_pSwapChain = nullptr;
	bool g_SwapChainOccluded = false;
	ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

	ImDrawList* draw_list = nullptr;
	ImFont* menu_font = new ImFont();
	ImFont* arial_font = new ImFont();
	ImFont* bold_font = new ImFont();
	//ImFont* weapon_font = new ImFont();

	ID3D11ShaderResourceView* bombTexture = nullptr;
	ID3D11ShaderResourceView* settingsTexture = nullptr;
	ID3D11ShaderResourceView* reloadTexture = nullptr;

	bool isFinish = true;

	template <typename T>
	inline VOID SafeRelease(T*& p) noexcept;

	bool CreateDeviceD3D() noexcept;
	void CleanupDeviceD3D() noexcept;
	void CreateRenderTarget() noexcept;
	void CleanupRenderTarget() noexcept;

	void Handler() noexcept;
	void Render() noexcept;
	void Menu() noexcept;

	void AutoAcceptMenu() noexcept;
	void TriggerMenu() noexcept;
	void BombTimerMenu() noexcept;
	void SniperCrosshairMenu() noexcept;
	void RGBCrosshairMenu() noexcept;
	void KeystrokesMenu() noexcept;
	void KnifeSwitchMenu() noexcept;
	void AutoPistolMenu() noexcept;
	void AntiAfkMenu() noexcept;
	void CustomBindsMenu() noexcept;
	void KillSayMenu() noexcept;
	void KillSoundMenu() noexcept;
	void RoundStartAlertMenu() noexcept;
	void SnapTapMenu() noexcept;
	void GradientManagerMenu() noexcept;
	void WatermarkMenu() noexcept;
	void FPSLimitMenu() noexcept;
	void CaptureBypassMenu() noexcept;

	//void RenderEsp() noexcept;

	FLOAT RenderText(ImFont* font, const std::string& text, const ImVec2& position, const float size, const ImColor& color, const bool centerX, const bool centerY, const bool outline, const bool background) noexcept;
	void RenderCrosshair(const std::optional<SteamTools::Crosshair>& ch);
	void CircularTimer(const ImVec2& pos, const float min, const float max, const float& time, const int points_count, const float radius, const float thickness, const bool top, const ImColor& color);
	ID3D11ShaderResourceView* LoadTextureFromPNGShellcode(unsigned char* icon_png, unsigned int icon_png_size);
	bool ImageButton(const char* str_id, ImTextureID imageTexture, const ImVec2& size);

	void OverlayLoop() noexcept;
};