#include "Overlay.h"

std::string FormatElapsedTime(double timeInMillis) {
	int totalMillis = std::lround(timeInMillis);
	int seconds = totalMillis / 1000;
	int millis = (totalMillis % 1000) / 100;

	std::stringstream formattedTime;
	formattedTime << std::setw(2) << std::setfill('0') << seconds << "."
				  << std::setw(1) << std::setfill('0') << millis;

	return formattedTime.str();
}

void Overlay::Render() noexcept {
    RGBColor gradient_color;

    if (cfg->watermarkState) {
        // Get FPS directly from ImGui
        //float fps = ImGui::GetIO().Framerate;

        // Get current time
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string timeStr = std::ctime(&now);
        timeStr = timeStr.substr(11, 8); // Extract "HH:MM:SS" (e.g., "14:30:45")

        // Get GMT offset (Windows-specific)
        TIME_ZONE_INFORMATION tzi;
        DWORD tzResult = GetTimeZoneInformation(&tzi);
        int offsetMinutes = 0;
        if (tzResult != TIME_ZONE_ID_INVALID) {
            offsetMinutes = -tzi.Bias; // Bias is minutes west of UTC, so negate for GMT offset
            if (tzResult == TIME_ZONE_ID_DAYLIGHT) {
                offsetMinutes -= tzi.DaylightBias; // Adjust for daylight saving time
            }
        }

        // Convert minutes to hours and determine GMT+ or GMT-
        int offsetHours = offsetMinutes / 60;
        std::string gmtOffset = " GMT";
        if (offsetHours > 0) {
            gmtOffset += "+" + std::to_string(offsetHours);
        }
        else if (offsetHours < 0) {
            gmtOffset += std::to_string(offsetHours); // Includes the negative sign
        }
        else {
            gmtOffset += "0";
        }

        // Append GMT offset to time string
        timeStr += gmtOffset;

        // Format watermark text
        std::string watermarkText = std::format(" ErScripts | {} | Ping {}ms | {}", globals::nickname, globals::cs2_ping, timeStr);

        ImGui::SetNextWindowBgAlpha(cfg->watermarkTransparency);

        if (!cfg->watermarkTransparency) ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        else ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
        ImGui::Begin(" Watermark", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
        ImGui::PushFont(arial_font);
        ImGui::SetWindowFontScale(16.0f / ImGui::GetFontSize());

        float watermarkSize = ImGui::CalcTextSize(watermarkText.c_str()).x + 20.0f;

        ImGui::SetWindowPos({ globals::width - watermarkSize - 6.0f, 5.0f });
        ImGui::SetWindowSize({ watermarkSize, 25 });

        // Use ImGui::Text() instead of AddText() for better UTF-8 handling
        gradient_color = gradient.getCurrentColor();
        ImColor color = cfg->watermarkGradientState ? ImColor(gradient_color.r, gradient_color.g, gradient_color.b) : ImColor(1.f, 1.f, 1.f);
        ImGui::TextColored(color, watermarkText.c_str());

        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopFont();
        ImGui::PopStyleVar(2);
        ImGui::End();
    }

    if (CS2Functions::GetCursorState()) {
		if (cfg->sniperCrosshairState && globals::sniperCrosshairState && !globals::isScope) {
			RenderCrosshair(*globals::crosshair);
		}
	}

    if (cfg->bombTimerState) {
        if (globals::bombTime || globals::menuState) {
            static bool isBombTimerWindowInitilized = false;

            ImGui::SetNextWindowBgAlpha(cfg->bombTimerTransparency);
            if (!isBombTimerWindowInitilized) {
                ImGui::SetNextWindowPos({ cfg->bombTimerPos[0], cfg->bombTimerPos[1] });
                isBombTimerWindowInitilized = true;
            }

            if (!cfg->bombTimerTransparency) ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            else ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f * cfg->bombTimerScale); // Scaled border size
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f * cfg->bombTimerScale); // Scaled rounding
            ImGui::Begin(" Bomb Timer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

            ImGui::SetWindowSize({ 146.0f * cfg->bombTimerScale, 70.0f * cfg->bombTimerScale }); // Scaled window size

            ImVec2 bombTimer_pos = ImGui::GetWindowPos();
            ImVec2 bombTimer_size = ImGui::GetWindowSize();

            if (bombTimer_pos.x < 0) {
                bombTimer_pos.x = 0;
            }
            if (bombTimer_pos.y < 0) {
                bombTimer_pos.y = 0;
            }
            if (bombTimer_pos.x + bombTimer_size.x > globals::width) {
                bombTimer_pos.x = globals::width - bombTimer_size.x;
            }
            if (bombTimer_pos.y + bombTimer_size.y > globals::height) {
                bombTimer_pos.y = globals::height - bombTimer_size.y;
            }

            if ((bombTimer_pos.x != ImGui::GetWindowPos().x || bombTimer_pos.y != ImGui::GetWindowPos().y) && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
                ImGui::SetWindowPos(" Bomb Timer", bombTimer_pos);
            }

            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 iconPos = ImVec2(windowPos.x + 5.0f * cfg->bombTimerScale, windowPos.y + (windowSize.y - 64.0f * cfg->bombTimerScale) / 2); // Scaled position and icon size
            ImVec2 c4IconSize = ImVec2(64.0f * cfg->bombTimerScale, 64.0f * cfg->bombTimerScale); // Scaled icon size

            gradient_color = gradient.getCurrentColor();
            ImColor color = cfg->bombTimerGradientState ? ImColor(gradient_color.r, gradient_color.g, gradient_color.b) : ImColor(1.f, 1.f, 1.f);
            ImGui::GetWindowDrawList()->AddImage(
                (ImTextureID)bombTexture,
                iconPos,
                ImVec2(iconPos.x + c4IconSize.x, iconPos.y + c4IconSize.y),
                ImVec2(0, 0), ImVec2(1, 1),
                ImColor(color)
            );

            // Position for circular timer (right side, centered vertically)
            ImVec2 timerCenter = ImVec2(
                windowPos.x + windowSize.x - 39.0f * cfg->bombTimerScale, // Scaled from right edge
                windowPos.y + windowSize.y / 2                            // Center vertically
            );

            // Set text color based on time remaining
            ImVec4 timeColor = ImVec4(1.0f, 0.3f, 0.0f, 1.0f); // Red default

            if (globals::bombTime) {
                if ((globals::defusekitState && globals::bombTime >= 5000) ||
                    (!globals::defusekitState && globals::bombTime >= 10000)) {
                    timeColor = ImVec4(0.3f, 1.0f, 0.0f, 1.0f); // Green when safe
                }

                // Draw circular timer with scaled parameters
                CircularTimer(timerCenter, 0.0f, 40000.0f, globals::bombTime, 32 * cfg->bombTimerScale, 22.0f * cfg->bombTimerScale, 4.0f * cfg->bombTimerScale, true, timeColor);

                // Center the time text below the timer with scaled font size
                std::string timeText = FormatElapsedTime(globals::bombTime);
                RenderText(bold_font, timeText, timerCenter, 14.0f * cfg->bombTimerScale, ImColor(timeColor), true, true, false, false);
            }
            else if (globals::menuState) {
                CircularTimer(timerCenter, 0.0f, 40000.0f, 10000.0f, 32 * cfg->bombTimerScale, 22.0f * cfg->bombTimerScale, 4.0f * cfg->bombTimerScale, true, timeColor);

                RenderText(bold_font, "10.0", timerCenter, 14.0f * cfg->bombTimerScale, ImColor(timeColor), true, true, false, false);
            }

            // Store window position
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
            ImGui::SetNextWindowPos({ cfg->keystrokesPos[0], cfg->keystrokesPos[1] });
            isKeystrokesWindowInitilized = true;
        }


        ImGui::Begin(" Keystrokes", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | (globals::menuState ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoBackground));
        
        ImGui::PushFont(arial_font);
        ImGui::SetWindowFontScale(cfg->keystrokesScale * 0.5f);

        // Style settings for a beautiful look, scaled
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * cfg->keystrokesScale); // Rounded corners
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * cfg->keystrokesScale, 4.0f * cfg->keystrokesScale)); // Padding inside buttons
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f * cfg->keystrokesScale, 6.0f * cfg->keystrokesScale)); // Spacing between elements

        gradient_color = gradient.getCurrentColor();
        ImGui::PushStyleColor(ImGuiCol_Text, cfg->keystrokesGradientState ? ImVec4(gradient_color.r / 255.0f, gradient_color.g / 255.0f, gradient_color.b / 255.0f, 1.0f) : ImVec4(1.f, 1.f, 1.f, 1.f));

        ImVec2 keystrokes_pos = ImGui::GetWindowPos();
        ImVec2 keystrokes_size = ImGui::GetWindowSize();

        if (keystrokes_pos.x < 0) {
            keystrokes_pos.x = 0;
        }
        if (keystrokes_pos.y < 0) {
            keystrokes_pos.y = 0;
        }
        if (keystrokes_pos.x + keystrokes_size.x > globals::width) {
            keystrokes_pos.x = globals::width - keystrokes_size.x;
        }
        if (keystrokes_pos.y + keystrokes_size.y > globals::height) {
            keystrokes_pos.y = globals::height - keystrokes_size.y;
        }

        if ((keystrokes_pos.x != ImGui::GetWindowPos().x || keystrokes_pos.y != ImGui::GetWindowPos().y) && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
            ImGui::SetWindowPos(" Keystrokes", keystrokes_pos);
        }

        // Key states with smooth transitions
        static float wAlpha = 0.0f, aAlpha = 0.0f, sAlpha = 0.0f, dAlpha = 0.0f, lmbAlpha = 0.0f, rmbAlpha = 0.0f;
        bool wPressed = GetAsyncKeyState('W') & 0x8000;
        bool aPressed = GetAsyncKeyState('A') & 0x8000;
        bool sPressed = GetAsyncKeyState('S') & 0x8000;
        bool dPressed = GetAsyncKeyState('D') & 0x8000;
        bool lmbPressed = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
        bool rmbPressed = GetAsyncKeyState(VK_RBUTTON) & 0x8000;

        ImVec4 releasedColor = ImVec4(cfg->keystrokesReleasedColor.r / 255.0f, cfg->keystrokesReleasedColor.g / 255.0f, cfg->keystrokesReleasedColor.b / 255.0f, cfg->keystrokesTransparency);
        ImVec4 pressedColor = ImVec4(cfg->keystrokesPressedColor.r / 255.0f, cfg->keystrokesPressedColor.g / 255.0f, cfg->keystrokesPressedColor.b / 255.0f, 1.0f);

        // Smoothly interpolate alpha values for animation
        wAlpha += (wPressed ? 1.0f - wAlpha : -wAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        aAlpha += (aPressed ? 1.0f - aAlpha : -aAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        sAlpha += (sPressed ? 1.0f - sAlpha : -sAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        dAlpha += (dPressed ? 1.0f - dAlpha : -dAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        lmbAlpha += (lmbPressed ? 1.0f - lmbAlpha : -lmbAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;
        rmbAlpha += (rmbPressed ? 1.0f - rmbAlpha : -rmbAlpha) * ImGui::GetIO().DeltaTime * cfg->keystrokesAnimationSpeed;

        // WASD Layout
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(0, 0)); ImGui::SameLine(36.0f * cfg->keystrokesScale); // Offset for centering
        ImGui::PushStyleColor(ImGuiCol_Button, ImLerp(releasedColor, pressedColor, wAlpha));
        ImGui::Button("W", ImVec2(30.0f * cfg->keystrokesScale, 30.0f * cfg->keystrokesScale));
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, ImLerp(releasedColor, pressedColor, aAlpha));
        ImGui::Button("A", ImVec2(30.0f * cfg->keystrokesScale, 30.0f * cfg->keystrokesScale)); ImGui::SameLine();
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, ImLerp(releasedColor, pressedColor, sAlpha));
        ImGui::Button("S", ImVec2(30.0f * cfg->keystrokesScale, 30.0f * cfg->keystrokesScale)); ImGui::SameLine();
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, ImLerp(releasedColor, pressedColor, dAlpha));
        ImGui::Button("D", ImVec2(30.0f * cfg->keystrokesScale, 30.0f * cfg->keystrokesScale));
        ImGui::PopStyleColor();
        ImGui::EndGroup();

        // Mouse Buttons
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Button, ImLerp(releasedColor, pressedColor, lmbAlpha));
        ImGui::Button("LMB", ImVec2(48.0f * cfg->keystrokesScale, 30.0f * cfg->keystrokesScale)); ImGui::SameLine();
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, ImLerp(releasedColor, pressedColor, rmbAlpha));
        ImGui::Button("RMB", ImVec2(48.0f * cfg->keystrokesScale, 30.0f * cfg->keystrokesScale));
        ImGui::PopStyleColor();
        ImGui::EndGroup();

        ImVec2 pos = ImGui::GetWindowPos();
        cfg->keystrokesPos[0] = pos.x;
        cfg->keystrokesPos[1] = pos.y;

        ImGui::SetWindowFontScale(1.f); // Restore the original font size
        ImGui::PopFont();               // Restore the original font
        ImGui::PopStyleVar(3);          // Restore style vars
        ImGui::PopStyleColor(1);        // Restore style colors
        ImGui::End();
    }}
