#include "app/app_version.h"
#include "core/color_gradient.hpp"
#include "core/config.hpp"
#include "core/globals.hpp"
#include "game/game_window.hpp"
#include "graphics/image_data.hpp"
#include "graphics/overlay.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <iomanip>
#include <sstream>

#include <imgui_internal.h>

std::string formatElapsedTime(double timeInMillis) {
    int totalMillis = std::lround(timeInMillis);
    int seconds = totalMillis / 1000;
    int millis = (totalMillis % 1000) / 100;

    std::stringstream formattedTime;
    formattedTime << std::setw(2) << std::setfill('0') << seconds << "." << std::setw(1) << std::setfill('0') << millis;

    return formattedTime.str();
}

void Overlay::renderWatermark() {
    if (!cfg->watermarkState)
        return;
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm local{};
    localtime_s(&local, &now);
    char clock[16]{};
    std::strftime(clock, sizeof(clock), "%H:%M:%S", &local);
    std::string text = "ErScripts " APP_VERSION;
    if (!globals::nickname.empty())
        text += " | " + globals::nickname;
    text += std::format(" | {} ms | {}", globals::cs2Ping.load(), clock);
    constexpr float fontSize = 15.f, paddingX = 10.f, paddingY = 6.f, margin = 8.f;
    const float available = std::max(1.f, globals::width - 2 * margin - 2 * paddingX);
    const char* end = nullptr;
    auto size = arialFont_->CalcTextSizeA(fontSize, available, 0, text.c_str(), nullptr, &end);
    if (end && *end) {
        text.resize(static_cast<size_t>(end - text.c_str()));
        text += "...";
        size = arialFont_->CalcTextSizeA(fontSize, FLT_MAX, 0, text.c_str());
    }
    const float width = std::min(size.x + 2 * paddingX, globals::width - 2 * margin);
    const ImVec2 start{std::max(margin, globals::width - width - margin), margin};
    const ImVec2 endPos{start.x + width, start.y + fontSize + 2 * paddingY};
    const int alpha = static_cast<int>(std::clamp(cfg->watermarkTransparency, 0.f, 1.f) * 255);
    drawList_->AddRectFilled(start, endPos, IM_COL32(16, 20, 25, alpha), 6.f);
    if (alpha)
        drawList_->AddRect(start, endPos, IM_COL32(65, 76, 86, alpha), 6.f);
    const auto rgb = colorGradient().getCurrentColor();
    const ImU32 color = cfg->watermarkGradientState
                            ? IM_COL32((rgb.r + 255) / 2, (rgb.g + 255) / 2, (rgb.b + 255) / 2, 255)
                            : IM_COL32(235, 239, 243, 255);
    drawList_->PushClipRect(start, endPos, true);
    drawList_->AddText(arialFont_, fontSize, {start.x + paddingX, start.y + paddingY}, color, text.c_str());
    drawList_->PopClipRect();
}

void Overlay::render() {
    const double bombTime = globals::bombTime.load();
    const auto game = globals::getGameState();
    const auto settings = globals::getConfig().value_or(SteamTools::Config{});
    const auto viewport = globals::getVideoSize();
    renderWatermark();
    RgbColor gradientColor;

    if (game::cursorHidden()) {
        if (!(GetAsyncKeyState(VK_TAB) & 0x8000)) {
            const auto frame =
                cfg->sniperCrosshairState ? globals::getSniperFrame(settings, viewport.height) : std::nullopt;
            if (frame && !globals::isScope) {
                renderCrosshair(settings, *frame, viewport);
            } else if (cfg->recoilCrosshairState && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                renderCrosshair(settings, {}, viewport);
            }
        }
    }

    if (cfg->bombTimerState) {
        if (bombTime || globals::menuState) {
            static bool isBombTimerWindowInitilized = false;

            ImGui::SetNextWindowBgAlpha(cfg->bombTimerTransparency);
            if (!isBombTimerWindowInitilized) {
                ImGui::SetNextWindowPos({cfg->bombTimerPos[0], cfg->bombTimerPos[1]});
                isBombTimerWindowInitilized = true;
            }

            if (!cfg->bombTimerTransparency)
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            else
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f * cfg->bombTimerScale);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f * cfg->bombTimerScale);
            ImGui::Begin(" Bomb Timer", nullptr,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoTitleBar);

            ImGui::SetWindowSize({146.0f * cfg->bombTimerScale, 70.0f * cfg->bombTimerScale});

            ImVec2 bombTimerPos = ImGui::GetWindowPos();
            ImVec2 bombTimerSize = ImGui::GetWindowSize();

            if (bombTimerPos.x < 0) {
                bombTimerPos.x = 0;
            }
            if (bombTimerPos.y < 0) {
                bombTimerPos.y = 0;
            }
            if (bombTimerPos.x + bombTimerSize.x > globals::width) {
                bombTimerPos.x = globals::width - bombTimerSize.x;
            }
            if (bombTimerPos.y + bombTimerSize.y > globals::height) {
                bombTimerPos.y = globals::height - bombTimerSize.y;
            }

            if ((bombTimerPos.x != ImGui::GetWindowPos().x || bombTimerPos.y != ImGui::GetWindowPos().y) &&
                (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
                ImGui::SetWindowPos(" Bomb Timer", bombTimerPos);
            }

            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 iconPos = ImVec2(windowPos.x + 5.0f * cfg->bombTimerScale,
                                    windowPos.y + (windowSize.y - 64.0f * cfg->bombTimerScale) / 2);
            ImVec2 c4IconSize = ImVec2(64.0f * cfg->bombTimerScale, 64.0f * cfg->bombTimerScale);

            gradientColor = colorGradient().getCurrentColor();
            ImColor color = cfg->bombTimerGradientState ? ImColor(gradientColor.r, gradientColor.g, gradientColor.b)
                                                        : ImColor(1.f, 1.f, 1.f);
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)bombTexture_, iconPos,
                                                 ImVec2(iconPos.x + c4IconSize.x, iconPos.y + c4IconSize.y),
                                                 ImVec2(0, 0), ImVec2(1, 1), ImColor(color));

            ImVec2 timerCenter =
                ImVec2(windowPos.x + windowSize.x - 39.0f * cfg->bombTimerScale, windowPos.y + windowSize.y / 2);

            ImVec4 timeColor = ImVec4(1.0f, 0.3f, 0.0f, 1.0f);
            ImDrawList* windowDrawList = ImGui::GetWindowDrawList();

            if (bombTime) {
                if ((game.defuseKit && bombTime >= 5000) || (!game.defuseKit && bombTime >= 10000)) {
                    timeColor = ImVec4(0.3f, 1.0f, 0.0f, 1.0f);
                }

                circularTimer(timerCenter, 0.0f, 40000.0f, static_cast<float>(bombTime),
                              static_cast<int>(32 * cfg->bombTimerScale), 22.0f * cfg->bombTimerScale,
                              4.0f * cfg->bombTimerScale, true, timeColor);

                std::string timeText = formatElapsedTime(bombTime);
                renderText(windowDrawList, boldFont_, timeText, timerCenter, 14.0f * cfg->bombTimerScale,
                           ImColor(timeColor), true, true, false, false);
            } else if (globals::menuState) {
                circularTimer(timerCenter, 0.0f, 40000.0f, 10000.0f, static_cast<int>(32 * cfg->bombTimerScale),
                              22.0f * cfg->bombTimerScale, 4.0f * cfg->bombTimerScale, true, timeColor);

                renderText(windowDrawList, boldFont_, "10.0", timerCenter, 14.0f * cfg->bombTimerScale,
                           ImColor(timeColor), true, true, false, false);
            }

            cfg->bombTimerPos[0] = windowPos.x;
            cfg->bombTimerPos[1] = windowPos.y;

            ImGui::PopStyleVar(2);
            ImGui::End();
        }
    }

    if (cfg->keystrokesState) {
        static bool isKeystrokesWindowInitilized = false;

        ImGui::SetNextWindowBgAlpha(0.5f);

        if (!isKeystrokesWindowInitilized) {
            ImGui::SetNextWindowPos({cfg->keystrokesPos[0], cfg->keystrokesPos[1]});
            isKeystrokesWindowInitilized = true;
        }

        ImGui::Begin(" Keystrokes", nullptr,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoTitleBar |
                         (globals::menuState ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoBackground));

        ImGui::PushFont(arialFont_);
        ImGui::SetWindowFontScale(cfg->keystrokesScale * 0.5f);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * cfg->keystrokesScale);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            ImVec2(4.0f * cfg->keystrokesScale, 4.0f * cfg->keystrokesScale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                            ImVec2(6.0f * cfg->keystrokesScale, 6.0f * cfg->keystrokesScale));

        gradientColor = colorGradient().getCurrentColor();
        ImGui::PushStyleColor(ImGuiCol_Text, cfg->keystrokesGradientState
                                                 ? ImVec4(gradientColor.r / 255.0f, gradientColor.g / 255.0f,
                                                          gradientColor.b / 255.0f, 1.0f)
                                                 : ImVec4(1.f, 1.f, 1.f, 1.f));

        ImVec2 keystrokesPos = ImGui::GetWindowPos();
        ImVec2 keystrokesSize = ImGui::GetWindowSize();

        if (keystrokesPos.x < 0) {
            keystrokesPos.x = 0;
        }
        if (keystrokesPos.y < 0) {
            keystrokesPos.y = 0;
        }
        if (keystrokesPos.x + keystrokesSize.x > globals::width) {
            keystrokesPos.x = globals::width - keystrokesSize.x;
        }
        if (keystrokesPos.y + keystrokesSize.y > globals::height) {
            keystrokesPos.y = globals::height - keystrokesSize.y;
        }

        if ((keystrokesPos.x != ImGui::GetWindowPos().x || keystrokesPos.y != ImGui::GetWindowPos().y) &&
            (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
            ImGui::SetWindowPos(" Keystrokes", keystrokesPos);
        }

        static float wAlpha = 0.0f, aAlpha = 0.0f, sAlpha = 0.0f, dAlpha = 0.0f, lmbAlpha = 0.0f, rmbAlpha = 0.0f;
        bool wPressed = GetAsyncKeyState('W') & 0x8000;
        bool aPressed = GetAsyncKeyState('A') & 0x8000;
        bool sPressed = GetAsyncKeyState('S') & 0x8000;
        bool dPressed = GetAsyncKeyState('D') & 0x8000;
        bool lmbPressed = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        bool rmbPressed = GetAsyncKeyState(VK_RBUTTON) & 0x8000;

        ImVec4 releasedColor = ImVec4(cfg->keystrokesReleasedColor.r / 255.0f, cfg->keystrokesReleasedColor.g / 255.0f,
                                      cfg->keystrokesReleasedColor.b / 255.0f, cfg->keystrokesTransparency);
        ImVec4 pressedColor = ImVec4(cfg->keystrokesPressedColor.r / 255.0f, cfg->keystrokesPressedColor.g / 255.0f,
                                     cfg->keystrokesPressedColor.b / 255.0f, 1.0f);

        wAlpha += (wPressed ? 1.0f - wAlpha : -wAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        aAlpha += (aPressed ? 1.0f - aAlpha : -aAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        sAlpha += (sPressed ? 1.0f - sAlpha : -sAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        dAlpha += (dPressed ? 1.0f - dAlpha : -dAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        lmbAlpha +=
            (lmbPressed ? 1.0f - lmbAlpha : -lmbAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        rmbAlpha +=
            (rmbPressed ? 1.0f - rmbAlpha : -rmbAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;

        const auto key = [&](const char* label, float width, float animation) {
            const auto color = ImLerp(releasedColor, pressedColor, std::clamp(animation, 0.f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
            ImGui::Button(label, {width * cfg->keystrokesScale, 30.f * cfg->keystrokesScale});
            ImGui::PopStyleColor(3);
        };
        ImGui::BeginGroup();
        ImGui::Dummy({0, 0});
        ImGui::SameLine(36.f * cfg->keystrokesScale);
        key("W", 30, wAlpha);
        key("A", 30, aAlpha);
        ImGui::SameLine();
        key("S", 30, sAlpha);
        ImGui::SameLine();
        key("D", 30, dAlpha);
        ImGui::EndGroup();
        ImGui::BeginGroup();
        key("LMB", 48, lmbAlpha);
        ImGui::SameLine();
        key("RMB", 48, rmbAlpha);
        ImGui::EndGroup();

        ImVec2 pos = ImGui::GetWindowPos();
        cfg->keystrokesPos[0] = pos.x;
        cfg->keystrokesPos[1] = pos.y;

        ImGui::SetWindowFontScale(1.f);
        ImGui::PopFont();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(1);
        ImGui::End();
    }
}
