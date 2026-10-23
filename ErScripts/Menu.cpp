#include "Overlay.h"
#include "CrosshairDrawing.h"
#include <unordered_map>
#include "ConfigExport.h"
#include "AppVersion.h"
#include <shellapi.h>
#include <commdlg.h>

namespace {
    constexpr ImVec4 accent(.38f, .85f, .65f, 1.f);
    constexpr ImVec4 muted(.52f, .58f, .65f, 1.f);
    ImGuiTextFilter search;
    int page = 0;
    bool *riskTarget = nullptr;
    std::string riskName;
    bool riskAccepted = false;
    bool riskOpen = false;
    bool details = false;

    const char *keyName(int key) {
        switch (key) {
        case 0:
            return "None";
        case VK_LBUTTON:
            return "Mouse 1";
        case VK_RBUTTON:
            return "Mouse 2";
        case VK_MBUTTON:
            return "Mouse 3";
        case VK_XBUTTON1:
            return "Mouse 4";
        case VK_XBUTTON2:
            return "Mouse 5";
        case VK_INSERT:
            return "Insert";
        case VK_END:
            return "End";
        }
        static char text[64];
        UINT scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
        if (key == VK_DELETE || key == VK_HOME || key == VK_PRIOR || key == VK_NEXT)
            scan |= 0x100;
        return GetKeyNameTextA(scan << 16, text, sizeof(text)) ? text : "None";
    }

    void hotkey(const char *label, int &binding) {
        static ImGuiID listening = 0;
        static double started = 0;
        ImGui::PushID(label);
        const auto id = ImGui::GetID("capture");
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(std::max(140.f, ImGui::GetWindowContentRegionMax().x - 160.f));
        if (ImGui::Button(listening == id ? "Press a key..." : keyName(binding), {150, 25})) {
            listening = id;
            started = ImGui::GetTime();
        }
        if (listening == id && ImGui::GetTime() - started > .2) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered())
                listening = 0;
            for (int key = 1; listening == id && key < 256; ++key) {
                if (key == VK_LWIN || key == VK_RWIN)
                    continue;
                if (GetAsyncKeyState(key) & 0x8000) {
                    binding = key == VK_ESCAPE || key == VK_BACK ? 0 : key;
                    listening = 0;
                }
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Escape clears the key. Click elsewhere to cancel.");
        ImGui::PopID();
    }
    void textField(const char *label, std::string &value) {
        char buffer[512]{};
        strncpy_s(buffer, value.c_str(), sizeof(buffer) - 1);
        ImGui::TextUnformatted(label);
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText((std::string("##") + label).c_str(), buffer, sizeof(buffer)))
            value = buffer;
    }
    bool risky(const char *name) {
        return std::string_view(name) == "Auto Stop" || std::string_view(name) == "Pixel Trigger" ||
               std::string_view(name) == "Angle Bind" || std::string_view(name) == "Auto Pistol";
    }
    bool feature(int category, const char *name, const char *description, bool *enabled = nullptr,
                 bool options = true) {
        if (search.IsActive() ? !search.PassFilter(name) && !search.PassFilter(description) : page != category)
            return false;
        ImGui::PushID(name);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {.075f, .09f, .11f, 1.f});
        ImGui::BeginChild("card", {0, 0},
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY |
                              ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar);
        static std::unordered_map<ImGuiID, bool> open;
        bool &expanded = open[ImGui::GetID("details")];
        if (enabled) {
            bool value = *enabled;
            if (ImGui::Checkbox(name, &value)) {
                if (value && risky(name)) {
                    riskTarget = enabled;
                    riskName = name;
                    riskAccepted = false;
                    riskOpen = true;
                } else
                    *enabled = value;
            }
        } else {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(name);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", description);
        if (options) {
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::GetFrameHeight());
            if (ImGui::ArrowButton("##settings", expanded ? ImGuiDir_Up : ImGuiDir_Down))
                expanded = !expanded;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", expanded ? "Collapse settings" : "Expand settings");
        }
        details = options && expanded;
        if (details) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }
        ImGui::PushItemWidth(-145);
        return true;
    }
    void endFeature() {
        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    void riskDialog() {
        if (riskOpen) {
            ImGui::OpenPopup("Before enabling");
            riskOpen = false;
        }
        const auto display = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowSize({std::min(450.f, display.x - 24), 0}, ImGuiCond_Always);
        ImGui::SetNextWindowPos({display.x * .5f, display.y * .5f}, ImGuiCond_Appearing, {.5f, .5f});
        bool open = true;
        if (ImGui::BeginPopupModal("Before enabling", &open,
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::TextColored(accent, "%s", riskName.c_str());
            ImGui::Spacing();
            ImGui::TextWrapped("This feature automates shooting, movement or aiming. It may violate game, server or "
                               "platform rules and may lead to restrictions or a ban.");
            ImGui::Spacing();
            ImGui::TextWrapped("External operation does not guarantee safety. You are responsible for where "
                               "and how you use it.");
            ImGui::Spacing();
            ImGui::Checkbox("I understand and accept the risk", &riskAccepted);
            ImGui::Spacing();
            if (ImGui::Button("Cancel", {100, 28})) {
                riskTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(!riskAccepted || !riskTarget);
            if (ImGui::Button("Enable at my own risk", {210, 28})) {
                *riskTarget = true;
                riskTarget = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();
            ImGui::EndPopup();
        }
        if (!open)
            riskTarget = nullptr;
    }
    void configExport() {
        auto &exporter = ConfigExport::instance();
        const auto state = exporter.status();
        ImGui::TextUnformatted("CS2 config");
        ImGui::Spacing();
        ImGui::BeginDisabled(state.busy);
        if (ImGui::Button("Export saved files", {180, 30}))
            exporter.start(false);
        ImGui::SameLine();
        ImGui::BeginDisabled(globals::previewMode);
        if (ImGui::Button("Capture from CS2", {180, 30})) {
            exporter.start(true);
            globals::menuState = false;
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::TextWrapped("CFG + binds, aliases, video and original settings files.");
        ImGui::TextDisabled("Live capture closes this menu. Return to CS2 to start.");
        ImGui::Spacing();
        if (!state.message.empty())
            ImGui::TextWrapped("%s", state.message.c_str());
        if (state.queued || state.capturing) {
            if (ImGui::Button("Cancel export"))
                exporter.cancel();
        }
        if (state.ready) {
            ImGui::Text("%zu settings / %zu binds / %zu aliases / %zu files", state.convars, state.binds, state.aliases,
                        state.files);
            if (ImGui::Button("Open export folder", {180, 28}))
                ShellExecuteW(nullptr, L"open", state.folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        if (ImGui::CollapsingHeader("Export details")) {
            ImGui::BeginDisabled(state.busy);
            if (ImGui::Button("Export another Steam account...")) {
                wchar_t file[MAX_PATH]{};
                OPENFILENAMEW dialog{};
                dialog.lStructSize = sizeof(dialog);
                dialog.hwndOwner = GetForegroundWindow();
                dialog.lpstrFilter = L"CS2 settings\0cs2_user_convars_0_slot0.vcfg\0VCFG files\0*.vcfg\0\0";
                dialog.lpstrFile = file;
                dialog.nMaxFile = MAX_PATH;
                dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
                if (GetOpenFileNameW(&dialog))
                    exporter.start(false, std::filesystem::path(file).parent_path());
            }
            ImGui::EndDisabled();
            ImGui::TextWrapped("Select that account's cs2_user_convars_0_slot0.vcfg to export its saved settings.");
            ImGui::Spacing();
            ImGui::TextWrapped("Saved files can lag behind unsaved changes. Live capture reads exposed "
                               "convars, binds and aliases. "
                               "Video and machine settings are kept as original files. Hidden engine state "
                               "cannot be exported.");
            ImGui::TextWrapped("Copy erscripts-export.cfg into game/csgo/cfg and run: exec erscripts-export");
        }
    }

    void crosshairEditor() {
        auto current = globals::getConfig();
        const auto viewport = globals::getVideoSize();
        static crosshair::Settings edit;
        static bool initialized = false, dirty = false;
        static float previewSpread = 0.f;
        static int previewBackground = 0;
        if (current && (!initialized || (!dirty && !globals::hasPendingCrosshair()))) {
            edit = *current;
            initialized = true;
        }

        if (!edit.loaded) {
            edit.screenHeight = std::clamp(viewport.height, 240, 65535);
            edit.modernFields = 15;
        }

        if (!current || !current->loaded)
            ImGui::TextDisabled("Defaults loaded. Reload reads your CS2 config.");
        if (!edit.modern())
            ImGui::TextWrapped("Legacy settings. Save the crosshair in CS2, then reload.");

        if (ImGui::BeginTable("studio", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) {
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch, 1.2f);
            ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 1.f);
            ImGui::TableNextColumn();
            ImGui::PushItemWidth(-1);
            dirty |= ImGui::Combo("##Style", &edit.style, crosshair::styles.data(),
                                  static_cast<int>(crosshair::styles.size()));
            ImGui::Spacing();
            const auto rendered = crosshair::dimensions(edit, viewport.height);
            const auto dimension = [&](const char *label, float &value, float low, float high, int actual, bool used) {
                ImGui::BeginDisabled(!used);
                ImGui::TextUnformatted(label);
                if (edit.modern() && edit.screenHeight != viewport.height) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%d px)", actual);
                }
                int pixels = static_cast<int>(value);
                if (ImGui::DragInt((std::string("##") + label).c_str(), &pixels, 1.f, static_cast<int>(low),
                                   static_cast<int>(high), "%d", ImGuiSliderFlags_AlwaysClamp)) {
                    value = static_cast<float>(pixels);
                    dirty = true;
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    if (!used)
                        ImGui::SetTooltip("This style does not use this setting.");
                    else
                        ImGui::SetTooltip("CS2 range: %.0f to %.0f. Rendered: %d px.", low, high, actual);
                }
                ImGui::EndDisabled();
            };
            const bool usesLength =
                edit.style == 0 || edit.style == 2 || edit.style == 4 || edit.style == 5 || edit.style == 7;
            const bool usesGap =
                edit.style == 2 || edit.style == 3 || edit.style == 4 || edit.style == 5 || edit.style >= 7;
            dimension("Length", edit.size, 0, crosshair::limits::lengthMax, rendered.length, usesLength);
            dimension("Thickness", edit.thickness, 0, crosshair::limits::thicknessMax, rendered.thickness, true);
            dimension("Gap", edit.gap, edit.style == 2 ? crosshair::limits::gapMin : 0.f, crosshair::limits::gapMax,
                      rendered.gap, usesGap);
            dirty |= ImGui::Checkbox("Center dot", &edit.dot);
            ImGui::SameLine();
            dirty |= ImGui::Checkbox("T shape", &edit.tStyle);
            dirty |= ImGui::Checkbox("Follow recoil", &edit.recoil);
            const auto color = [&](const char *label, int &r, int &g, int &b, int &a) {
                float rgba[] = {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
                if (ImGui::ColorEdit4(label, rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs)) {
                    r = static_cast<int>(rgba[0] * 255.f + .5f);
                    g = static_cast<int>(rgba[1] * 255.f + .5f);
                    b = static_cast<int>(rgba[2] * 255.f + .5f);
                    a = static_cast<int>(rgba[3] * 255.f + .5f);
                    dirty = true;
                }
            };
            color("Color", edit.red, edit.green, edit.blue, edit.alpha);
            const char *outlines[] = {"No outline", "Full outline", "Half outline"};
            dirty |= ImGui::Combo("##Outline", &edit.drawOutline, outlines, 3);
            color("Outline color", edit.outlineRed, edit.outlineGreen, edit.outlineBlue, edit.outlineAlpha);
            ImGui::PopItemWidth();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("Preview");
            const ImVec2 start = ImGui::GetCursorScreenPos();
            const float width = ImGui::GetContentRegionAvail().x;
            const float height = 170.f;
            auto *draw = ImGui::GetWindowDrawList();
            const ImU32 backgrounds[] = {IM_COL32(17, 23, 30, 255), IM_COL32(204, 208, 204, 255),
                                         IM_COL32(101, 92, 73, 255)};
            draw->AddRectFilled(start, {start.x + width, start.y + height}, backgrounds[previewBackground], 10);
            for (float x = 16; x < width; x += 20)
                draw->AddLine({start.x + x, start.y}, {start.x + x, start.y + height}, IM_COL32(128, 128, 128, 18));
            for (float y = 15; y < height; y += 20)
                draw->AddLine({start.x, start.y + y}, {start.x + width, start.y + y}, IM_COL32(128, 128, 128, 18));
            draw->PushClipRect(start, {start.x + width, start.y + height}, true);
            crosshair::draw(draw, edit, {start.x + width * .5f, start.y + height * .5f}, viewport.height, previewSpread,
                            {float(globals::width) / viewport.width, float(globals::height) / viewport.height});
            draw->PopClipRect();
            ImGui::Dummy({width, height});
            ImGui::SetNextItemWidth(-1);
            const char *backgroundsText[] = {"Midnight", "Light", "Sand"};
            ImGui::Combo("##Backdrop", &previewBackground, backgroundsText, 3);
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("##PreviewSpread", &previewSpread, 0, 64, "Spread %.0f px");
            ImGui::TextDisabled("CS2: %d x %d", viewport.width, viewport.height);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Overlay: %d x %d pixels", globals::width, globals::height);

            ImGui::EndTable();
        }
        if (ImGui::CollapsingHeader("Dynamic")) {
            dirty |= ImGui::SliderInt("Spread limit", &edit.dynamicSpreadLimit, 0, crosshair::limits::spreadMax, "%d",
                                      ImGuiSliderFlags_AlwaysClamp);
            dirty |= ImGui::SliderFloat("Split distance", &edit.dynamicSplitDist, 0, crosshair::limits::splitMax,
                                        "%.0f px", ImGuiSliderFlags_AlwaysClamp);
            dirty |= ImGui::SliderFloat("Inner alpha", &edit.dynamicSplitAlphaInnerMod, 0, 1, "%.2f");
            dirty |= ImGui::SliderFloat("Outer alpha", &edit.dynamicSplitAlphaOuterMod, .3f, 1, "%.2f");
            dirty |= ImGui::SliderFloat(edit.style == 9 ? "Quadrant arc ratio" : "Outer length ratio",
                                        &edit.dynamicMaxDistSplitRatio, 0, 1, "%.2f");
        }
        if (ImGui::CollapsingHeader("Scope / grenades")) {
            dirty |= ImGui::SliderFloat("Scope dot scale", &edit.scopeDotScale, crosshair::limits::scopeMin,
                                        crosshair::limits::scopeMax, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            dirty |= ImGui::Checkbox("Use crosshair color for scope dot", &edit.scopeUseCrosshairColor);
            if (ImGui::InputInt("Sniper line width", &edit.sniperWidth)) {
                edit.sniperWidth = std::max(1, edit.sniperWidth);
                dirty = true;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("CS2 has no explicit upper bound for this setting.");
            dirty |= ImGui::Checkbox("Keep crosshair while throwing", &edit.grenadeKeepCrosshair);
            dirty |= ImGui::Checkbox("Grenade tick labels", &edit.grenadeTickLabels);
            dirty |=
                ImGui::SliderFloat("Grenade tick scale", &edit.grenadeTickScaling, crosshair::limits::grenadeScaleMin,
                                   crosshair::limits::grenadeScaleMax, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            const char *grenades[] = {"Decoy", "HE grenade", "Molotov / incendiary", "Flashbang", "Smoke"};
            for (int i = 0; i < 5; ++i) {
                ImGui::PushID(i);
                dirty |= ImGui::Checkbox(grenades[i], &edit.grenadeEnabled[i]);
                ImGui::SameLine(220);
                ImGui::SetNextItemWidth(150);
                dirty |= ImGui::SliderFloat("##Delay", &edit.grenadeDelay[i], 0, crosshair::limits::grenadeDelayMax,
                                            "%.1f sec", ImGuiSliderFlags_AlwaysClamp);
                ImGui::PopID();
            }
        }
        ImGui::Spacing();
        if (ImGui::Button("Apply to CS2", {150, 34})) {
            edit.loaded = true;
            edit.modernFields = 15;
            if (globals::previewMode)
                globals::setConfig(edit);
            else
                globals::queueCrosshair(edit);
            dirty = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Copy commands", {140, 34}))
            ImGui::SetClipboardText(crosshair::commands(edit).c_str());
        ImGui::SameLine();
        if (ImGui::Button("Reload", {85, 34})) {
            if (auto loaded = SteamTools::getCrosshairSettings("730")) {
                globals::setConfig(*loaded);
                edit = *loaded;
                dirty = false;
            }
        }
        if (dirty) {
            ImGui::SameLine();
            ImGui::TextColored(accent, "Unsaved draft");
        }
        if (globals::hasPendingCrosshair())
            ImGui::TextColored(accent, "Queued. Close the menu to apply.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
} // namespace

void Overlay::Menu() noexcept {
    const auto previousStyle = ImGui::GetStyle();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 10;
    style.ChildRounding = 10;
    style.FrameRounding = 6;
    style.GrabRounding = 5;
    style.PopupRounding = 10;
    style.TabRounding = 6;
    style.TabBorderSize = 0;
    style.TabBarBorderSize = 1;
    style.TabBarOverlineSize = 0;
    style.ScrollbarRounding = 8;
    style.WindowPadding = {12, 12};
    style.FramePadding = {8, 4};
    style.ItemSpacing = {8, 6};
    style.ItemInnerSpacing = {8, 6};
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.Colors[ImGuiCol_WindowBg] = {.045f, .055f, .07f, 1};
    style.Colors[ImGuiCol_ChildBg] = {.06f, .075f, .09f, 1};
    style.Colors[ImGuiCol_PopupBg] = {.08f, .10f, .12f, 1};
    style.Colors[ImGuiCol_TitleBg] = {.06f, .075f, .09f, 1};
    style.Colors[ImGuiCol_TitleBgActive] = {.10f, .17f, .18f, 1};
    style.Colors[ImGuiCol_TitleBgCollapsed] = {.06f, .075f, .09f, 1};
    style.Colors[ImGuiCol_ModalWindowDimBg] = {0, 0, 0, .60f};
    style.Colors[ImGuiCol_TextSelectedBg] = {.38f, .85f, .65f, .28f};
    style.Colors[ImGuiCol_Border] = {.15f, .19f, .23f, 1};
    style.Colors[ImGuiCol_Text] = {.91f, .94f, .96f, 1};
    style.Colors[ImGuiCol_TextDisabled] = muted;
    style.Colors[ImGuiCol_FrameBg] = {.11f, .14f, .17f, 1};
    style.Colors[ImGuiCol_FrameBgHovered] = {.16f, .21f, .25f, 1};
    style.Colors[ImGuiCol_FrameBgActive] = {.19f, .27f, .29f, 1};
    style.Colors[ImGuiCol_Button] = {.13f, .19f, .21f, 1};
    style.Colors[ImGuiCol_ButtonHovered] = {.20f, .31f, .31f, 1};
    style.Colors[ImGuiCol_ButtonActive] = {.24f, .39f, .35f, 1};
    style.Colors[ImGuiCol_Header] = {.13f, .25f, .23f, 1};
    style.Colors[ImGuiCol_HeaderHovered] = {.17f, .30f, .27f, 1};
    style.Colors[ImGuiCol_HeaderActive] = {.20f, .36f, .30f, 1};
    style.Colors[ImGuiCol_Tab] = {.08f, .105f, .13f, 1};
    style.Colors[ImGuiCol_TabHovered] = {.16f, .27f, .26f, 1};
    style.Colors[ImGuiCol_TabSelected] = {.13f, .25f, .23f, 1};
    style.Colors[ImGuiCol_TabDimmed] = style.Colors[ImGuiCol_Tab];
    style.Colors[ImGuiCol_TabDimmedSelected] = style.Colors[ImGuiCol_TabSelected];
    style.Colors[ImGuiCol_NavCursor] = accent;
    style.Colors[ImGuiCol_CheckMark] = accent;
    style.Colors[ImGuiCol_SliderGrab] = accent;
    style.Colors[ImGuiCol_SliderGrabActive] = {.56f, .95f, .77f, 1};
    style.Colors[ImGuiCol_Separator] = {.14f, .18f, .21f, 1};
    style.Colors[ImGuiCol_ResizeGrip] = {.38f, .85f, .65f, .22f};

    const ImVec2 available = ImGui::GetIO().DisplaySize;
    const ImVec2 size(std::min(820.f, available.x - 24), std::min(610.f, available.y - 24));
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowPos({available.x * .5f, available.y * .5f}, ImGuiCond_FirstUseEver, {.5f, .5f});
    ImGui::PushFont(menu_font);
    if (!ImGui::Begin("ErScripts##Main", nullptr,
                      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        ImGui::PopFont();
        ImGui::GetStyle() = previousStyle;
        return;
    }
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(accent, "ErScripts");
    ImGui::SameLine();
    ImGui::TextDisabled(APP_VERSION);
    constexpr float searchWidth = 215.f, headerButtonWidth = 52.f;
    const float headerWidth = searchWidth + 2 * headerButtonWidth + 2 * style.ItemSpacing.x;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - headerWidth);
    ImGui::SetNextItemWidth(searchWidth);
    if (ImGui::InputTextWithHint("##Search", "Search", search.InputBuf, IM_ARRAYSIZE(search.InputBuf)))
        search.Build();
    ImGui::SameLine();
    if (ImGui::Button("Hide", {headerButtonWidth, 25}))
        globals::menuState = false;
    ImGui::SameLine();
    if (ImGui::Button("Exit", {headerButtonWidth, 25}))
        isFinish = false;
    ImGui::Spacing();
    const char *names[] = {"General", "Crosshair", "Overlay", "Audio / chat", "Settings", "Config"};
    if (ImGui::BeginTabBar("sections")) {
        for (int i = 0; i < IM_ARRAYSIZE(names); ++i) {
            if (ImGui::BeginTabItem(names[i])) {
                if (page != i) {
                    page = i;
                    search.Clear();
                    ImGui::SetScrollY(0);
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    static int previousPage = -1;
    ImGui::BeginChild("content", {0, -40}, ImGuiChildFlags_AlwaysUseWindowPadding);
    if (previousPage != page) {
        ImGui::SetScrollY(0);
        previousPage = page;
    }
    if (page == 1 && !search.IsActive())
        crosshairEditor();
    if (page == 5 && !search.IsActive())
        configExport();
    if (feature(0, "Auto Accept", "Match found. Accept after your chosen delay.", &cfg->autoAcceptState)) {
        if (details)
            ImGui::SliderInt("Delay", &cfg->autoAcceptWaitingTime, 1, 30, "%d sec");
        endFeature();
    }
    if (feature(1, "Sniper Crosshair", "Show your crosshair while a sniper is unscoped.", &cfg->sniperCrosshairState)) {
        if (details) {

            const auto player = globals::getPlayerState();
            if (player.sniper()) {
                const auto name = player.weapon.starts_with("weapon_") ? player.weapon.substr(7) : player.weapon;
                ImGui::Text("%s%s", name.c_str(), player.reloading ? " / reloading" : "");
                if (player.clip >= 0 && player.reserve >= 0)
                    ImGui::Text("Ammo: %d / %d", player.clip, player.reserve);
                ImGui::Text("Shots: %llu", static_cast<unsigned long long>(player.confirmedShots));
            } else {
                ImGui::TextDisabled("No active sniper rifle.");
            }
        }
        endFeature();
    }
    if (feature(1, "Recoil Crosshair", "Keep a static reference while CS2 follows recoil.", &cfg->recoilCrosshairState,
                false)) {
        if (details)
            ImGui::TextWrapped("The overlay remains centered. CS2 handles the recoil movement.");
        endFeature();
    }
    if (feature(1, "RGB Crosshair", "Animate your crosshair with the shared gradient.", &cfg->rgbCrosshairState,
                false)) {
        if (details)
            ImGui::TextWrapped("Tune the color sequence in Preferences / Gradient.");
        endFeature();
    }
    if (feature(2, "Bomb Timer", "A countdown with defuse status on your overlay.", &cfg->bombTimerState)) {
        if (details) {
            ImGui::SliderFloat("Scale", &cfg->bombTimerScale, .5f, 2, "%.2f");
            ImGui::SliderFloat("Opacity", &cfg->bombTimerTransparency, 0, 1, "%.2f");
            ImGui::Checkbox("Gradient icon", &cfg->bombTimerGradientState);
        }
        endFeature();
    }
    if (feature(2, "Keystrokes", "A compact view of your keyboard and mouse input.", &cfg->keystrokesState)) {
        if (details) {
            ImGui::SliderFloat("Scale", &cfg->keystrokesScale, 1, 2, "%.2f");
            ImGui::SliderFloat("Animation speed", &cfg->keystrokesAnimationSpeed, 1, 50, "%.1f");
            ImGui::SliderFloat("Opacity", &cfg->keystrokesTransparency, 0, 1, "%.2f");
            ImGui::Checkbox("Gradient text", &cfg->keystrokesGradientState);
            const auto rgb = [](const char *label, RGBColor &c) {
                float v[] = {c.r / 255.f, c.g / 255.f, c.b / 255.f};
                if (ImGui::ColorEdit3(label, v, ImGuiColorEditFlags_NoInputs))
                    c = {int(v[0] * 255), int(v[1] * 255), int(v[2] * 255)};
            };
            rgb("Pressed", cfg->keystrokesPressedColor);
            rgb("Released", cfg->keystrokesReleasedColor);
        }
        endFeature();
    }
    if (feature(2, "Watermark", "Session name, clock and ping at a glance.", &cfg->watermarkState)) {
        if (details) {
            ImGui::Checkbox("Gradient text", &cfg->watermarkGradientState);
            ImGui::SliderFloat("Opacity", &cfg->watermarkTransparency, 0, 1, "%.2f");
            ImGui::SliderInt("Ping refresh", &cfg->watermarkPingUpdateRate, 1, 10, "%d sec");
        }
        endFeature();
    }
    if (feature(0, "Pixel Trigger", "Configure the existing pixel action.", &cfg->triggerState)) {
        if (details) {
            hotkey("Hotkey", cfg->triggerBind);
            ImGui::SliderInt("Threshold", &cfg->triggerThreshold, 0, 100);
            ImGui::SliderInt2("Offset X / Y", cfg->triggerDisplacement, 0, 20);
            ImGui::SliderInt("Delay", &cfg->triggerDelayMs, 0, 100, "%d ms");
        }
        endFeature();
    }
    if (feature(0, "Auto Stop", "Counter input on your configured hotkey.", &cfg->autoStopState)) {
        if (details) {
            hotkey("Hotkey", cfg->autoStopBind);
            ImGui::Checkbox("Toggle mode", &cfg->autoStopToggleState);
        }
        endFeature();
    }
    if (feature(0, "Angle Bind", "Rotate your view by a configured angle.", &cfg->angleBindState)) {
        if (details) {
            hotkey("Hotkey", cfg->angleBindBind);
            ImGui::SliderFloat("Angle", &cfg->angleBindDegree, -180, 180, "%.2f degrees");
        }
        endFeature();
    }
    if (feature(0, "Knife Switch", "Switch hands when your active weapon changes.", &cfg->knifeSwitchState, false)) {
        if (details)
            ImGui::TextWrapped("Uses the built-in switchhands command.");
        endFeature();
    }
    if (feature(0, "Auto Pistol", "Repeat the existing pistol action.", &cfg->autoPistolState, false)) {
        if (details)
            ImGui::TextWrapped("Automation requires explicit consent.");
        endFeature();
    }
    if (feature(0, "Anti AFK", "Keep the existing idle input routine enabled.", &cfg->antiAfkState, false)) {
        if (details)
            ImGui::TextWrapped("Stops when the feature is disabled.");
        endFeature();
    }
    if (feature(3, "Kill Sound", "Play a sound when the kill counter increases.", &cfg->killSoundState)) {
        if (details) {
            ImGui::SliderInt("Volume", &cfg->killSoundVolume, 1, 100, "%d%%");
            textField("WAV file", cfg->killSoundFileName);
            ImGui::TextDisabled("Empty uses the built-in sound.");
        }
        endFeature();
    }
    if (feature(3, "Round Start Alert", "Hear the round begin while CS2 is in the background.",
                &cfg->roundStartAlertState)) {
        if (details) {
            ImGui::SliderInt("Volume", &cfg->roundStartAlertVolume, 1, 100, "%d%%");
            textField("WAV file", cfg->roundStartAlertFileName);
        }
        endFeature();
    }
    if (feature(3, "Kill Say", "Send your configured message after a kill.", &cfg->killSayState)) {
        if (details)
            textField("Message", cfg->killSayText);
        endFeature();
    }
    if (feature(3, "Chat Spammer", "Manage the existing repeated chat message.", &cfg->chatSpammerState)) {
        if (details) {
            textField("Message", cfg->chatSpammerText);
            hotkey("Hotkey", cfg->chatSpammerBind);
        }
        endFeature();
    }
    if (feature(4, "Performance", "Balance overlay refresh rate and VSync.", &cfg->fpsLimiterState)) {
        if (details) {
            ImGui::Checkbox("VSync", &cfg->vsyncState);
            ImGui::BeginDisabled(cfg->vsyncState);
            ImGui::SliderInt("Frame limit", &cfg->fpsLimiter, 60, 500, "%d FPS");
            ImGui::EndDisabled();
        }
        endFeature();
    }
    if (feature(4, "Capture Bypass", "Control the existing overlay capture option.", &cfg->captureBypassState, false)) {
        if (details)
            ImGui::TextWrapped("Applies to the ErScripts overlay window.");
        endFeature();
    }
    if (feature(4, "Gradient", "One color sequence for your crosshair and overlay.")) {
        if (details) {
            bool changed = ImGui::SliderInt("Steps", &cfg->gradient.numSteps, 50, 2000);
            changed |= ImGui::SliderInt("Step delay", &cfg->gradient.delayMs, 1, 100, "%d ms");
            changed |= ImGui::SliderFloat("Start hue", &cfg->gradient.startHue, 0, 1, "%.2f");
            changed |= ImGui::SliderFloat("End hue", &cfg->gradient.endHue, 0, 1, "%.2f");
            changed |= ImGui::SliderFloat("Saturation", &cfg->gradient.saturation, 0, 1, "%.2f");
            changed |= ImGui::SliderFloat("Brightness", &cfg->gradient.value, 0, 1, "%.2f");
            if (changed)
                gradient.setConfig(cfg->gradient);
            const auto c = gradient.getCurrentColor();
            ImGui::ColorButton("Current color", ImColor(c.r, c.g, c.b), 0, {140, 26});
        }
        endFeature();
    }
    if (feature(4, "App shortcuts", "Choose the menu and exit keys.")) {
        if (details) {
            hotkey("Menu", cfg->erScriptsMenuBind);
            hotkey("Exit", cfg->erScriptsExitBind);
        }
        endFeature();
    }
    if (feature(0, "Custom Binds", "Keep movement, utility and app shortcuts together.")) {
        if (details) {
            hotkey("Long Jump", cfg->longJumpBind);
            hotkey("Jump Throw", cfg->jumpThrowBind);
            hotkey("Drop Bomb", cfg->dropBombBind);
            hotkey("Self Kick", cfg->selfKickBind);
        }
        endFeature();
    }
    ImGui::EndChild();
    ImGui::Separator();
    static double savedAt = -100;
    if (ImGui::Button("Save", {85, 28})) {
        cfg->save("default");
        savedAt = ImGui::GetTime();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load", {85, 28})) {
        cfg->load("default");
        gradient.setConfig(cfg->gradient);
        if (ImGui::FindWindowByName(" Bomb Timer"))
            ImGui::SetWindowPos(" Bomb Timer", {cfg->bombTimerPos[0], cfg->bombTimerPos[1]});
        if (ImGui::FindWindowByName(" Keystrokes"))
            ImGui::SetWindowPos(" Keystrokes", {cfg->keystrokesPos[0], cfg->keystrokesPos[1]});
    }
    ImGui::SameLine();
    if (ImGui::GetTime() - savedAt < 3)
        ImGui::TextColored(accent, "Saved");
    else
        ImGui::TextDisabled("default");
    riskDialog();
    ImGui::End();
    ImGui::PopFont();
    ImGui::GetStyle() = previousStyle;
}
