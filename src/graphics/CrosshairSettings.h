#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace crosshair {
    inline constexpr std::array<const char *, 10> styles = {
        "Dynamic Cross", "Dynamic Circle", "Dynamic Cross (Legacy)",
        "Static Circle", "Static Cross",   "Static Cross (Shot Feedback)",
        "Dot Only",      "Dynamic Quad",   "Static Square",
        "Static Quad"};

    namespace limits {
        inline constexpr float lengthMax = 255.f, thicknessMax = 32.f;
        inline constexpr float gapMin = -3840.f, gapMax = 3840.f, splitMax = 127.f;
        inline constexpr int spreadMax = 255;
        inline constexpr float scopeMin = .1f, scopeMax = 2.f;
        inline constexpr float grenadeScaleMin = .5f, grenadeScaleMax = 2.f, grenadeDelayMax = 5.f;
    } // namespace limits

    struct Settings {
        float gap = 4.f;
        float size = 8.f;
        float thickness = 2.f;
        int screenHeight = 1080;
        int red = 0, green = 255, blue = 0, alpha = 255;
        int outlineRed = 0, outlineGreen = 0, outlineBlue = 0, outlineAlpha = 255;
        int drawOutline = 1;
        int style = 7;
        bool dot = false, tStyle = false, recoil = false;
        float dynamicSplitDist = 3.f;
        float dynamicSplitAlphaInnerMod = 0.f, dynamicSplitAlphaOuterMod = 1.f;
        float dynamicMaxDistSplitRatio = 1.f;
        int dynamicSpreadLimit = 255;
        float scopeDotScale = 1.f;
        bool scopeUseCrosshairColor = false;
        int sniperWidth = 1;
        bool grenadeKeepCrosshair = false, grenadeTickLabels = true;
        float grenadeTickScaling = 1.f;
        std::array<bool, 5> grenadeEnabled = {true, true, true, true, true};
        std::array<float, 5> grenadeDelay = {2.f, 2.f, 2.f, 2.f, 2.f};
        float sensitivity = 1.25f, zoomSensitivity = 1.f, yaw = .022f, pitch = .022f;
        float outlineThickness = 1.f;
        int color = 5;
        bool useAlpha = true, gapUseWeaponValue = false;
        float fixedCrosshairGap = 3.f;
        bool loaded = false;
        unsigned modernFields = 0;

        bool isEmpty() const noexcept { return !loaded; }
        bool modern() const noexcept { return (modernFields & 7u) != 0; }
    };

    inline std::optional<float> number(std::string_view value) {
        if (value == "true")
            return 1.f;
        if (value == "false")
            return 0.f;
        float result = 0.f;
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !std::isfinite(result))
            return std::nullopt;
        return result;
    }

    inline bool apply(Settings &s, std::string_view name, std::string_view value) {
        const auto parsed = number(value);
        if (!parsed)
            return false;
        const float v = *parsed;
        const auto channel = [&] { return static_cast<int>(std::clamp(v, 0.f, 255.f)); };
        const auto setModern = [&](unsigned bit, float &field, float min, float max) {
            field = std::clamp(v, min, max);
            s.modernFields |= bit;
        };
        if (name == "cl_crosshair_length")
            setModern(1, s.size, 0.f, limits::lengthMax);
        else if (name == "cl_crosshair_gap")
            setModern(2, s.gap, limits::gapMin, limits::gapMax);
        else if (name == "cl_crosshair_thickness")
            setModern(4, s.thickness, 0.f, limits::thicknessMax);
        else if (name == "cl_crosshaircolor_a") {
            s.alpha = channel();
            s.modernFields |= 8;
        } else if (name == "cl_crosshair_screen_height")
            s.screenHeight = static_cast<int>(std::clamp(v, 240.f, 65535.f));
        else if (name == "cl_crosshaircolor_r")
            s.red = channel();
        else if (name == "cl_crosshaircolor_g")
            s.green = channel();
        else if (name == "cl_crosshaircolor_b")
            s.blue = channel();
        else if (name == "cl_crosshairoutline_r")
            s.outlineRed = channel();
        else if (name == "cl_crosshairoutline_g")
            s.outlineGreen = channel();
        else if (name == "cl_crosshairoutline_b")
            s.outlineBlue = channel();
        else if (name == "cl_crosshairoutline_a")
            s.outlineAlpha = channel();
        else if (name == "cl_crosshair_drawoutline")
            s.drawOutline = static_cast<int>(std::clamp(v, 0.f, 2.f));
        else if (name == "cl_crosshairstyle")
            s.style = static_cast<int>(std::clamp(v, 0.f, 9.f));
        else if (name == "cl_crosshairdot")
            s.dot = v != 0.f;
        else if (name == "cl_crosshair_t")
            s.tStyle = v != 0.f;
        else if (name == "cl_crosshair_recoil")
            s.recoil = v != 0.f;
        else if (name == "cl_crosshair_dynamic_splitdist")
            s.dynamicSplitDist = std::clamp(v, 0.f, limits::splitMax);
        else if (name == "cl_crosshair_dynamic_splitalpha_innermod")
            s.dynamicSplitAlphaInnerMod = std::clamp(v, 0.f, 1.f);
        else if (name == "cl_crosshair_dynamic_splitalpha_outermod")
            s.dynamicSplitAlphaOuterMod = std::clamp(v, .3f, 1.f);
        else if (name == "cl_crosshair_dynamic_maxdist_splitratio")
            s.dynamicMaxDistSplitRatio = std::clamp(v, 0.f, 1.f);
        else if (name == "cl_crosshair_dynamic_spread_limit")
            s.dynamicSpreadLimit = static_cast<int>(std::clamp(v, 0.f, float(limits::spreadMax)));
        else if (name == "cl_ironsight_dot_scale")
            s.scopeDotScale = std::clamp(v, limits::scopeMin, limits::scopeMax);
        else if (name == "cl_ironsight_usecrosshaircolor")
            s.scopeUseCrosshairColor = v != 0.f;
        else if (name == "cl_crosshair_sniper_width") {
            int width = 1;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), width);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
                return false;
            s.sniperWidth = std::max(1, width);
        } else if (name == "cl_grenadecrosshair_keepusercrosshair")
            s.grenadeKeepCrosshair = v != 0.f;
        else if (name == "cl_grenadecrosshair_ticklabels")
            s.grenadeTickLabels = v != 0.f;
        else if (name == "cl_grenadecrosshair_tickscaling")
            s.grenadeTickScaling = std::clamp(v, limits::grenadeScaleMin, limits::grenadeScaleMax);
        else if (name == "sensitivity")
            s.sensitivity = std::clamp(v, .0001f, 1000.f);
        else if (name == "zoom_sensitivity_ratio")
            s.zoomSensitivity = std::clamp(v, .0001f, 1000.f);
        else if (name == "m_yaw")
            s.yaw = std::clamp(v, -1.f, 1.f);
        else if (name == "m_pitch")
            s.pitch = std::clamp(v, -1.f, 1.f);
        else if (name == "cl_crosshairsize") {
            if (!(s.modernFields & 1))
                s.size = std::clamp(v, 0.f, 255.f);
        } else if (name == "cl_crosshairgap") {
            if (!(s.modernFields & 2))
                s.gap = std::clamp(v, -255.f, 255.f);
        } else if (name == "cl_crosshairthickness") {
            if (!(s.modernFields & 4))
                s.thickness = std::clamp(v, 0.f, 255.f);
        } else if (name == "cl_crosshairalpha") {
            if (!(s.modernFields & 8))
                s.alpha = channel();
        } else if (name == "cl_crosshaircolor")
            s.color = static_cast<int>(std::clamp(v, 0.f, 5.f));
        else if (name == "cl_crosshair_outlinethickness")
            s.outlineThickness = std::clamp(v, 0.f, 3.f);
        else if (name == "cl_crosshairusealpha")
            s.useAlpha = v != 0.f;
        else if (name == "cl_crosshairgap_useweaponvalue")
            s.gapUseWeaponValue = v != 0.f;
        else if (name == "cl_fixedcrosshairgap")
            s.fixedCrosshairGap = std::clamp(v, -255.f, 255.f);
        else {
            constexpr std::array<std::string_view, 5> types = {"decoy", "explosive", "fire", "flash", "smoke"};
            for (size_t i = 0; i < types.size(); ++i) {
                if (name == std::string("cl_grenadecrosshair_") + std::string(types[i])) {
                    s.grenadeEnabled[i] = v != 0.f;
                    s.loaded = true;
                    return true;
                }
                if (name == std::string("cl_grenadecrosshairdelay_") + std::string(types[i])) {
                    s.grenadeDelay[i] = std::clamp(v, 0.f, limits::grenadeDelayMax);
                    s.loaded = true;
                    return true;
                }
            }
            return false;
        }
        if (name.starts_with("cl_crosshair"))
            s.loaded = true;
        return true;
    }

    inline std::string commands(const Settings &s) {
        std::ostringstream out;
        const auto add = [&](const char *name, auto value) { out << name << ' ' << value << ";\n"; };
        add("cl_crosshairstyle", s.style);
        add("cl_crosshair_length", s.size);
        add("cl_crosshair_gap", s.gap);
        add("cl_crosshair_thickness", s.thickness);
        add("cl_crosshair_screen_height", s.screenHeight);
        add("cl_crosshaircolor_r", s.red);
        add("cl_crosshaircolor_g", s.green);
        add("cl_crosshaircolor_b", s.blue);
        add("cl_crosshaircolor_a", s.alpha);
        add("cl_crosshair_drawoutline", s.drawOutline);
        add("cl_crosshairoutline_r", s.outlineRed);
        add("cl_crosshairoutline_g", s.outlineGreen);
        add("cl_crosshairoutline_b", s.outlineBlue);
        add("cl_crosshairoutline_a", s.outlineAlpha);
        add("cl_crosshairdot", s.dot);
        add("cl_crosshair_t", s.tStyle);
        add("cl_crosshair_recoil", s.recoil);
        add("cl_crosshair_dynamic_splitdist", s.dynamicSplitDist);
        add("cl_crosshair_dynamic_splitalpha_innermod", s.dynamicSplitAlphaInnerMod);
        add("cl_crosshair_dynamic_splitalpha_outermod", s.dynamicSplitAlphaOuterMod);
        add("cl_crosshair_dynamic_maxdist_splitratio", s.dynamicMaxDistSplitRatio);
        add("cl_crosshair_dynamic_spread_limit", s.dynamicSpreadLimit);
        add("cl_crosshair_sniper_width", s.sniperWidth);
        add("cl_ironsight_dot_scale", s.scopeDotScale);
        add("cl_ironsight_usecrosshaircolor", s.scopeUseCrosshairColor);
        add("cl_grenadecrosshair_keepusercrosshair", s.grenadeKeepCrosshair);
        add("cl_grenadecrosshair_ticklabels", s.grenadeTickLabels);
        add("cl_grenadecrosshair_tickscaling", s.grenadeTickScaling);
        constexpr std::array<const char *, 5> types = {"decoy", "explosive", "fire", "flash", "smoke"};
        for (size_t i = 0; i < types.size(); ++i) {
            add((std::string("cl_grenadecrosshair_") + types[i]).c_str(), s.grenadeEnabled[i]);
            add((std::string("cl_grenadecrosshairdelay_") + types[i]).c_str(), s.grenadeDelay[i]);
        }
        return out.str();
    }
} // namespace crosshair
