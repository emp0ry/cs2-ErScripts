#include "Overlay.h"

// Get key name from virtual key code
const char* GetKeyName(int virtualKeyCode) {
    static char keyName[128] = { 0 };

    switch (virtualKeyCode) {
        case VK_LBUTTON:    return "LMOUSE";
        case VK_RBUTTON:    return "RMOUSE";
        case VK_MBUTTON:    return "MOUSE3";
        case VK_XBUTTON1:   return "MOUSE4";
        case VK_XBUTTON2:   return "MOUSE5";
        case VK_END:        return "END";
        case VK_HOME:       return "HOME";
        case VK_INSERT:     return "INSERT";
        case VK_DELETE:     return "DELETE";
        case VK_PRIOR:      return "PAGE UP";
        case VK_NEXT:       return "PAGE DOWN";
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_SELECT:
        case VK_PRINT:
        case VK_EXECUTE:
        case VK_SNAPSHOT:
        case VK_HELP:
        case VK_LWIN:
        case VK_RWIN:
        case VK_APPS:
        case VK_SCROLL:
        case VK_ESCAPE:
        case VK_NUMLOCK:
            return "None";
    }

    UINT scanCode = MapVirtualKeyA(virtualKeyCode, MAPVK_VK_TO_VSC);
    if (GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName)) > 0) {
        return keyName;
    }
    return "None";
}

void Hotkey(bool* state, const char** key_name, int* key) {
    if (*state) {
        for (int i = 1; i < 166; i++) {
            if (GetAsyncKeyState(i) < 0) {
                *key_name = GetKeyName(i);
                *key = i;
                if (*key_name == "None")
                    *key = 0;
                *state = false;
                break;
            }
        }
    }
}

inline const char* triggerKeyName = GetKeyName(cfg->triggerBind);
inline bool triggerButtonState = false;
inline const char* longJumpKeyName = GetKeyName(cfg->longJumpBind);
inline bool longJumpButtonState = false;
inline const char* jumpThrowKeyName = GetKeyName(cfg->jumpThrowBind);
inline bool jumpThrowButtonState = false;
inline const char* dropBombKeyName = GetKeyName(cfg->dropBombBind);
inline bool dropBombButtonState = false;
inline const char* selfKickKeyName = GetKeyName(cfg->selfKickBind);
inline bool selfKickButtonState = false;
inline const char* autoStopKeyName = GetKeyName(cfg->autoStopBind);
inline bool autoStopButtonState = false;
inline const char* erScriptsMenuKeyName = GetKeyName(cfg->erScriptsMenuBind);
inline bool erScriptsMenuButtonState = false;
inline const char* erScriptsExitKeyName = GetKeyName(cfg->erScriptsExitBind);
inline bool erScriptsExitButtonState = false;
inline const char* chatSpammerKeyName = GetKeyName(cfg->chatSpammerBind);
inline bool chatSpammerButtonState = false;
inline const char* angleBindKeyName = GetKeyName(cfg->angleBindBind);
inline bool angleBindButtonState = false;
inline char killSayText[256]{};
inline char chatSpammerText[256]{};
inline char killSoundFileName[256]{};
inline char roundStartAlertFileName[256]{};

void Overlay::Menu() noexcept {
    // Get the current window title
    char window_title[32];
    if (!GetConsoleTitleA(window_title, sizeof(window_title)))
        window_title[0] = '\0';

    // Custom style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);                           // Rounded window corners
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);                             // Rounded frames (slider background)
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f);                              // Rounded slider grab handle
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));                 // More spacing
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));             // Inner padding
    // Colors
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));          // Dark background
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));            // Frame background
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.9f));             // Button background
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));      // Button hover
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));               // Light text
    // Titlebar colors
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));         // Inactive titlebar
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));   // Active titlebar

    // Slider grab color (to match your image's blue accent)
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.0f, 0.5f, 1.0f, 0.9f));         // Blue grab handle
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.0f, 0.6f, 1.2f, 1.0f));   // Brighter blue when active

    ImGui::PushFont(menu_font);
    ImGui::Begin(std::format("  {}", window_title).c_str(), &isFinish, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

    static ImVec2 menu_size = { 213.f, 272.f };

    static bool one_time = true;
    if (one_time) {
        ImGui::SetWindowSize(menu_size);

        ImGui::SetWindowPos(std::format("  {}", window_title).c_str(), {globals::width / 2 - menu_size.x / 2, globals::height / 2 - menu_size.y / 2});
        one_time = false;
    }

    // Clamp the window position to stay within screen boundaries
    ImVec2 menu_pos = ImGui::GetWindowPos();

    if (menu_pos.x < 0) {
        menu_pos.x = 0;
    }
    if (menu_pos.y < 0) {
        menu_pos.y = 0;
    }
    if (menu_pos.x + menu_size.x > globals::width) {
        menu_pos.x = globals::width - menu_size.x;
    }
    if (menu_pos.y + menu_size.y > globals::height) {
        menu_pos.y = globals::height - menu_size.y;
    }

    if ((menu_pos.x != ImGui::GetWindowPos().x || menu_pos.y != ImGui::GetWindowPos().y) && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
        ImGui::SetWindowPos(std::format("  {}", window_title).c_str(), menu_pos);
    }

    if (ImGui::BeginTable("Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
        AutoAcceptMenu();
        //TriggerMenu();
        BombTimerMenu();
        SniperCrosshairMenu();
        RecoilCrosshairMenu();
        RGBCrosshairMenu();
        KeystrokesMenu();
        KnifeSwitchMenu();
        AutoPistolMenu();
        AntiAfkMenu();
        CustomBindsMenu();
        KillSayMenu();
        KillSoundMenu();
        RoundStartAlertMenu();
        AutoStopMenu();
        ChatSpammerMenu();
        AngleBindMenu();
        GradientManagerMenu();
        WatermarkMenu();
        FPSLimitMenu();
        CaptureBypassMenu();

        ImGui::EndTable();
    }
    // Config
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("Save Config", { 83.5f, 28.f }))
        cfg->save("default");
    ImGui::SameLine();
    if (ImGui::Button("Load Config", { 83.5f, 28.f })) {
        cfg->load("default");
        gradient.setConfig(cfg->gradient);

        triggerKeyName = GetKeyName(cfg->triggerBind);
        longJumpKeyName = GetKeyName(cfg->longJumpBind);
        jumpThrowKeyName = GetKeyName(cfg->jumpThrowBind);
        dropBombKeyName = GetKeyName(cfg->dropBombBind);
        selfKickKeyName = GetKeyName(cfg->selfKickBind);
        autoStopKeyName = GetKeyName(cfg->autoStopBind);
        erScriptsMenuKeyName = GetKeyName(cfg->erScriptsMenuBind);
        erScriptsExitKeyName = GetKeyName(cfg->erScriptsExitBind);
        chatSpammerKeyName = GetKeyName(cfg->chatSpammerBind);
        angleBindKeyName = GetKeyName(cfg->angleBindBind);

        if (ImGui::FindWindowByName(" Bomb Timer"))
        ImGui::SetWindowPos(" Bomb Timer", { cfg->bombTimerPos[0], cfg->bombTimerPos[1] });

        if (ImGui::FindWindowByName(" Keystrokes"))
        ImGui::SetWindowPos(" Keystrokes", { cfg->keystrokesPos[0], cfg->keystrokesPos[1] });

        strncpy_s(killSayText, cfg->killSayText.c_str(), sizeof(killSayText) - 1);
        strncpy_s(chatSpammerText, cfg->chatSpammerText.c_str(), sizeof(chatSpammerText) - 1);
        strncpy_s(killSoundFileName, cfg->killSoundFileName.c_str(), sizeof(killSoundFileName) - 1);
        strncpy_s(roundStartAlertFileName, cfg->roundStartAlertFileName.c_str(), sizeof(roundStartAlertFileName) - 1);
    }

    ImGui::End();
    ImGui::PopFont();

    ImGui::PopStyleVar(5); // Pop all style vars
    ImGui::PopStyleColor(9); // Pop all style colors
}

void Overlay::AutoAcceptMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Auto Accept             ", &cfg->autoAcceptState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##AutoAccept", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Auto Accept Settings");
    }

    if (ImGui::BeginPopup("Auto Accept Settings")) {
        if (ImGui::BeginTable("Auto Accept Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Waiting Time ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            ImGui::SliderInt("##WaitingTime", &cfg->autoAcceptWaitingTime, 1, 30, "%d sec");
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::TriggerMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    ImGui::Checkbox("Pixel Trigger", &cfg->triggerState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##PixelTrigger", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Pixel Trigger Settings");
    }

    if (ImGui::BeginPopup("Pixel Trigger Settings", ImGuiWindowFlags_NoResize)) {
        if (ImGui::BeginTable("Pixel Trigger Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            // Hotkey
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Hotkey ");
            ImGui::TableSetColumnIndex(1);
            triggerKeyName = GetKeyName(cfg->triggerBind);
            if (ImGui::Button(std::format("{}##PixelTrigger", triggerKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                triggerButtonState = !triggerButtonState;
            }
            Hotkey(&triggerButtonState, &triggerKeyName, &cfg->triggerBind);

            // Threshold Slider
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Threshold ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100.0f);
            ImGui::SliderInt("##PixelTriggerThreshold", &cfg->triggerThreshold, 0, 100);
            ImGui::PopItemWidth();

            // Displacement Sliders (X and Y)
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Displacement ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(46.f);
            ImGui::SliderInt("##PixelTriggerDisplacementX", &cfg->triggerDisplacement[0], 0, 20, "%d x");
            ImGui::SameLine();
            ImGui::SliderInt("##PixelTriggerDisplacementY", &cfg->triggerDisplacement[1], 0, 20, "%d y");
            ImGui::PopItemWidth();

            // Delay
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Delay ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100.0f);
            ImGui::SliderInt("##PixelTriggerDelay", &cfg->triggerDelayMs, 0, 100, "%d ms");
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::BombTimerMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Bomb Timer", &cfg->bombTimerState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##BombTimer", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Bomb Time Settings");
    }

    if (ImGui::BeginPopup("Bomb Time Settings")) {
        if (ImGui::BeginTable("Bomb Time Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Scale ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("##BombTimerScale", &cfg->bombTimerScale, 0.5f, 2.0f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Gradient Color ");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##BombTimerGradient", &cfg->bombTimerGradientState);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Transparency ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("##BombTimerTransparency", &cfg->bombTimerTransparency, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::SniperCrosshairMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Sniper Crosshair", &cfg->sniperCrosshairState);
    ImGui::TableSetColumnIndex(1);
    //if (ImageButton("##SniperCrosshairReloader", (ImTextureID)reloadTexture, { 22, 22 })) {
    //    globals::configUpdaterState = true;
    //}
}

void Overlay::RecoilCrosshairMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Recoil Crosshair", &cfg->recoilCrosshairState);
    ImGui::TableSetColumnIndex(1);
    //if (ImageButton("##RecoilCrosshairReloader", (ImTextureID)reloadTexture, { 22, 22 })) {
    //    globals::configUpdaterState = true;
    //}
}

void Overlay::RGBCrosshairMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("RGB Crosshair", &cfg->rgbCrosshairState);
}

void Overlay::KeystrokesMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Keystrokes", &cfg->keystrokesState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##Keystrokes", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Keystrokes Settings");
    }

    if (ImGui::BeginPopup("Keystrokes Settings")) {
        if (ImGui::BeginTable("Keystrokes Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            //RGBColor keystrokesPressedColor{ 255, 80, 0 };
            //RGBColor keystrokesReleasedColor{ 80, 80, 80 };
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Scale ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("##KeystrokesScale", &cfg->keystrokesScale, 1.0f, 2.0f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Animation Speed ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("##KeystrokesSpeed", &cfg->keystrokesAnimationSpeed, 1.0f, 50.0f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Pressed Color ");
            ImGui::TableSetColumnIndex(1);
            float pressedColor[3] = {
                cfg->keystrokesPressedColor.r / 255.0f,
                cfg->keystrokesPressedColor.g / 255.0f,
                cfg->keystrokesPressedColor.b / 255.0f
            };
            ImGui::PushItemWidth(130);
            if (ImGui::ColorEdit3("##KeystrokesPressedColor", pressedColor)) {
                cfg->keystrokesPressedColor.r = static_cast<int>(pressedColor[0] * 255.0f);
                cfg->keystrokesPressedColor.g = static_cast<int>(pressedColor[1] * 255.0f);
                cfg->keystrokesPressedColor.b = static_cast<int>(pressedColor[2] * 255.0f);
            }
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Released Color ");
            ImGui::TableSetColumnIndex(1);
            float releasedColor[3] = {
                cfg->keystrokesReleasedColor.r / 255.0f,
                cfg->keystrokesReleasedColor.g / 255.0f,
                cfg->keystrokesReleasedColor.b / 255.0f
            };
            ImGui::PushItemWidth(130);
            if (ImGui::ColorEdit3("##KeystrokesReleasedColor", releasedColor)) {
                cfg->keystrokesReleasedColor.r = static_cast<int>(releasedColor[0] * 255.0f);
                cfg->keystrokesReleasedColor.g = static_cast<int>(releasedColor[1] * 255.0f);
                cfg->keystrokesReleasedColor.b = static_cast<int>(releasedColor[2] * 255.0f);
            }
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Transparency ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("##KeystrokesTransparency", &cfg->keystrokesTransparency, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Gradient Color ");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##KeystrokesGradient", &cfg->keystrokesGradientState);

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::KnifeSwitchMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Knife Switch", &cfg->knifeSwitchState);
}

void Overlay::AutoPistolMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Auto Pistol", &cfg->autoPistolState);
}

void Overlay::AntiAfkMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Anti Afk", &cfg->antiAfkState);
}

void Overlay::CustomBindsMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    RGBColor currentColor = gradient.getCurrentColor();
    ImGui::ColorButton("##CustomBindsXD", ImColor(currentColor.r, currentColor.g, currentColor.b), 0, ImVec2(23, 23));
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 5);
    ImGui::Text("Custom Binds");
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##CustomBinds", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("CustomBinds Settings");
    }

    if (ImGui::BeginPopup("CustomBinds Settings")) {
        if (ImGui::BeginTable("CustomBinds Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            /* Long Jump */
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Long Jump Bind ");
            ImGui::TableSetColumnIndex(1);
            longJumpKeyName = GetKeyName(cfg->longJumpBind);
            if (ImGui::Button(std::format("{}##LongJump", longJumpKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                longJumpButtonState = !longJumpButtonState;
            }
            Hotkey(&longJumpButtonState, &longJumpKeyName, &cfg->longJumpBind);

            /* Jump Throw */
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Jump Throw Bind ");
            ImGui::TableSetColumnIndex(1);
            jumpThrowKeyName = GetKeyName(cfg->jumpThrowBind);
            if (ImGui::Button(std::format("{}##JumpThrow", jumpThrowKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                jumpThrowButtonState = !jumpThrowButtonState;
            }
            Hotkey(&jumpThrowButtonState, &jumpThrowKeyName, &cfg->jumpThrowBind);

            /* Drop Bomb */
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Drop Bomb Bind ");
            ImGui::TableSetColumnIndex(1);
            dropBombKeyName = GetKeyName(cfg->dropBombBind);
            if (ImGui::Button(std::format("{}##DropBomb", dropBombKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                dropBombButtonState = !dropBombButtonState;
            }
            Hotkey(&dropBombButtonState, &dropBombKeyName, &cfg->dropBombBind);

            /* Self Kick */
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Self Kick Bind ");
            ImGui::TableSetColumnIndex(1);
            selfKickKeyName = GetKeyName(cfg->selfKickBind);
            if (ImGui::Button(std::format("{}##SelfKick", selfKickKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                selfKickButtonState = !selfKickButtonState;
            }
            Hotkey(&selfKickButtonState, &selfKickKeyName, &cfg->selfKickBind);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TableSetColumnIndex(1);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            /* ErScripts Binds */
            /* Menu */
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Menu Bind ");
            ImGui::TableSetColumnIndex(1);
            erScriptsMenuKeyName = GetKeyName(cfg->erScriptsMenuBind);
            if (ImGui::Button(std::format("{}##MenuBind", erScriptsMenuKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                erScriptsMenuButtonState = !erScriptsMenuButtonState;
            }
            Hotkey(&erScriptsMenuButtonState, &erScriptsMenuKeyName, &cfg->erScriptsMenuBind);

            /* Exit */
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Exit Bind ");
            ImGui::TableSetColumnIndex(1);
            erScriptsExitKeyName = GetKeyName(cfg->erScriptsExitBind);
            if (ImGui::Button(std::format("{}##ExitBind", erScriptsExitKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                erScriptsExitButtonState = !erScriptsExitButtonState;
            }
            Hotkey(&erScriptsExitButtonState, &erScriptsExitKeyName, &cfg->erScriptsExitBind);

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::KillSayMenu() noexcept {
    static bool firstRun = true;
    if (firstRun) {
        strncpy_s(killSayText, cfg->killSayText.c_str(), sizeof(killSayText) - 1);
        firstRun = false;
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Kill Say", &cfg->killSayState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##KillSay", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Kill Say Settings");
    }

    if (ImGui::BeginPopup("Kill Say Settings")) {
        if (ImGui::BeginTable("Kill Say Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Chat Message ");
            ImGui::TableSetColumnIndex(1);

            ImGui::PushItemWidth(150);

            ImGui::InputText("##KillSayText", killSayText, sizeof(killSayText), ImGuiInputTextFlags_EnterReturnsTrue);
            cfg->killSayText = killSayText;
            
            ImGui::PopItemWidth();
            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::KillSoundMenu() noexcept {
    static bool firstRun = true;
    if (firstRun) {
        strncpy_s(killSoundFileName, cfg->killSoundFileName.c_str(), sizeof(killSoundFileName) - 1);
        firstRun = false;
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Kill Sound", &cfg->killSoundState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##KillSound", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Kill Sound Settings");
    }

    if (ImGui::BeginPopup("Kill Sound Settings")) {
        if (ImGui::BeginTable("Kill Sound Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Volume ");
            ImGui::TableSetColumnIndex(1);

            ImGui::PushItemWidth(100);

            ImGui::SliderInt("##KillSoundVolume", &cfg->killSoundVolume, 1, 100, "%d %");

            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("File Name ");
            ImGui::TableSetColumnIndex(1);

            ImGui::PushItemWidth(150);

            ImGui::InputText("##KillSoundFileName", killSoundFileName, sizeof(killSoundFileName), ImGuiInputTextFlags_EnterReturnsTrue);
            cfg->killSoundFileName = killSoundFileName;
            
            ImGui::PopItemWidth();
            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::RoundStartAlertMenu() noexcept {
    static bool firstRun = true;
    if (firstRun) {
        strncpy_s(roundStartAlertFileName, cfg->roundStartAlertFileName.c_str(), sizeof(roundStartAlertFileName) - 1);
        firstRun = false;
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Round Start Alert", &cfg->roundStartAlertState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##RoundStartAlert", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Round Start Alert Settings");
    }

    if (ImGui::BeginPopup("Round Start Alert Settings")) {
        if (ImGui::BeginTable("Round Start Alert Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Volume ");
            ImGui::TableSetColumnIndex(1);

            ImGui::PushItemWidth(100);

            ImGui::SliderInt("##RoundStartAlertVolume", &cfg->roundStartAlertVolume, 1, 100, "%d %");

            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("File Name ");
            ImGui::TableSetColumnIndex(1);

            ImGui::PushItemWidth(150);

            ImGui::InputText("##RoundStartAlertFileName", roundStartAlertFileName, sizeof(roundStartAlertFileName), ImGuiInputTextFlags_EnterReturnsTrue);
            cfg->roundStartAlertFileName = roundStartAlertFileName;

            ImGui::PopItemWidth();
            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::AutoStopMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Auto Stop", &cfg->autoStopState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##AutoStop", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("AutoStop Settings");
    }

    if (ImGui::BeginPopup("AutoStop Settings")) {
        if (ImGui::BeginTable("AutoStop Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Hotkey ");
            ImGui::TableSetColumnIndex(1);
            autoStopKeyName = GetKeyName(cfg->autoStopBind);
            if (ImGui::Button(std::format("{}##PixelTrigger", autoStopKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                autoStopButtonState = !autoStopButtonState;
            }
            Hotkey(&autoStopButtonState, &autoStopKeyName, &cfg->autoStopBind);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Toggle ");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##AutoStopToggle", &cfg->autoStopToggleState);

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::ChatSpammerMenu() noexcept {
    static bool firstRun = true;
    if (firstRun) {
        strncpy_s(chatSpammerText, cfg->chatSpammerText.c_str(), sizeof(chatSpammerText) - 1);
        firstRun = false;
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Chat Spammer", &cfg->chatSpammerState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##ChatSpammer", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Chat Spammer Settings");
    }

    if (ImGui::BeginPopup("Chat Spammer Settings")) {
        if (ImGui::BeginTable("Chat Spammer Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Chat Message ");
            ImGui::TableSetColumnIndex(1);

            ImGui::PushItemWidth(150);

            ImGui::InputText("##ChatSpammerText", chatSpammerText, sizeof(chatSpammerText), ImGuiInputTextFlags_EnterReturnsTrue);
            cfg->chatSpammerText = chatSpammerText;

            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Hotkey ");
            ImGui::TableSetColumnIndex(1);
            chatSpammerKeyName = GetKeyName(cfg->chatSpammerBind);
            if (ImGui::Button(std::format("{}##ChatSpammerBind", chatSpammerKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                chatSpammerButtonState = !chatSpammerButtonState;
            }
            Hotkey(&chatSpammerButtonState, &chatSpammerKeyName, &cfg->chatSpammerBind);

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::AngleBindMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    ImGui::Checkbox("Angle Bind", &cfg->angleBindState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##AngleBind", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Angle Bind Settings");
    }

    if (ImGui::BeginPopup("Angle Bind Settings", ImGuiWindowFlags_NoResize)) {
        if (ImGui::BeginTable("Angle Bind Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            // Hotkey
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Hotkey ");
            ImGui::TableSetColumnIndex(1);
            angleBindKeyName = GetKeyName(cfg->angleBindBind);
            if (ImGui::Button(std::format("{}##PixelTrigger", angleBindKeyName).c_str(), ImVec2(80.0f, 22.0f))) {
                angleBindButtonState = !angleBindButtonState;
            }
            Hotkey(&angleBindButtonState, &angleBindKeyName, &cfg->angleBindBind);

            // Degree
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Degree ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100.0f);
            ImGui::SliderFloat("##AngleBindDegree", &cfg->angleBindDegree, -180.f, 180.f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::FPSLimitMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("FPS Limiter", &cfg->fpsLimiterState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##FPSLimiter", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("FPS Limiter Settings");
    }

    if (ImGui::BeginPopup("FPS Limiter Settings")) {
        if (ImGui::BeginTable("FPS Limiter Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("VSync ");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##VSync", &cfg->vsyncState);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("FPS Limit ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            if (cfg->vsyncState) ImGui::BeginDisabled();
            ImGui::SliderInt("##FpsLimiterSlide", &cfg->fpsLimiter, 60, 500, "%d fps");
            if (cfg->vsyncState) ImGui::EndDisabled();
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::WatermarkMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Watermark", &cfg->watermarkState);
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##Watermark", (ImTextureID)settingsTexture, { 22, 22 })) {
        ImGui::OpenPopup("Watermark Settings");
    }

    if (ImGui::BeginPopup("Watermark Settings")) {
        if (ImGui::BeginTable("Watermark Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Gradient Color ");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##WatermarkGradient", &cfg->watermarkGradientState);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Transparency ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("##WatermarkTransparency", &cfg->watermarkTransparency, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Ping Update Rate ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            ImGui::SliderInt("##WatermarkPingUpdateRateSlide", &cfg->watermarkPingUpdateRate, 1, 10, "%d sec");
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void Overlay::GradientManagerMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    RGBColor currentColor = gradient.getCurrentColor();
    ImGui::ColorButton("##GradientManagerPreview", ImColor(currentColor.r, currentColor.g, currentColor.b), 0, ImVec2(23, 23));
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 5);
    ImGui::Text("Gradient Manager");
    ImGui::TableSetColumnIndex(1);

    if (ImageButton("##gradient", (ImTextureID)settingsTexture, { 23, 23 })) {
        ImGui::OpenPopup("Gradient Settings");
    }

    if (ImGui::BeginPopup("Gradient Settings")) {
        if (ImGui::BeginTable("Gradient Settings Table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX)) {
            // Color Preview
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Preview ");
            ImGui::TableSetColumnIndex(1);
            ImGui::ColorButton("##GradientPreview", ImColor(currentColor.r, currentColor.g, currentColor.b), 0, ImVec2(100, 24));

            // Number of Steps
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Steps ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            int numSteps = cfg->gradient.numSteps;
            if (ImGui::SliderInt("##GradientSteps", &numSteps, 50, 2000, "%d")) {
                cfg->gradient.numSteps = numSteps;
                gradient.setConfig(cfg->gradient);
            }
            ImGui::PopItemWidth();

            // Delay Between Steps
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Delay ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            int delayMs = cfg->gradient.delayMs;
            if (ImGui::SliderInt("##GradientDelay", &delayMs, 1, 100, "%d ms")) {
                cfg->gradient.delayMs = delayMs;
                gradient.setConfig(cfg->gradient);
            }
            ImGui::PopItemWidth();

            // Start Hue
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Start Hue ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            float startHue = cfg->gradient.startHue;
            if (ImGui::SliderFloat("##StartHue", &startHue, 0.0f, 1.0f, "%.2f")) {
                cfg->gradient.startHue = startHue;
                gradient.setConfig(cfg->gradient);
            }
            ImGui::PopItemWidth();

            // End Hue
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("End Hue ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            float endHue = cfg->gradient.endHue;
            if (ImGui::SliderFloat("##EndHue", &endHue, 0.0f, 1.0f, "%.2f")) {
                cfg->gradient.endHue = endHue;
                gradient.setConfig(cfg->gradient);
            }
            ImGui::PopItemWidth();

            // Saturation
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Saturation ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            float saturation = cfg->gradient.saturation;
            if (ImGui::SliderFloat("##Saturation", &saturation, 0.0f, 1.0f, "%.2f")) {
                cfg->gradient.saturation = saturation;
                gradient.setConfig(cfg->gradient);
            }
            ImGui::PopItemWidth();

            // Value (Brightness)
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Value ");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(100);
            float value = cfg->gradient.value;
            if (ImGui::SliderFloat("##Value", &value, 0.0f, 1.0f, "%.2f")) {
                cfg->gradient.value = value;
                gradient.setConfig(cfg->gradient);
            }
            ImGui::PopItemWidth();

            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

void Overlay::CaptureBypassMenu() noexcept {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Checkbox("Capture Bypass", &cfg->captureBypassState);
}