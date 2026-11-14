#pragma once
#include <array>
#include <string_view>

namespace crosshair {
    struct SniperProfile {
        std::string_view name;
        float spread, standing, fire, recovery, cycle, recoil;
        unsigned magazine;
    };
    inline constexpr std::string_view sniperDataHash =
        "cc04aa26fd8d4a34119df7c4b7f4f29c7ebe0c7862ce78e0a2a753642502ba0a";
    inline constexpr std::array<SniperProfile, 4> sniperProfiles = {
        {{"weapon_awp", 0.0002f, 0.0808f, 0.05385f, 0.34539f, 1.455f, 78.0f, 5},
         {"weapon_ssg08", 0.00028f, 0.0317f, 0.02292f, 0.142096f, 1.25f, 33.0f, 10},
         {"weapon_scar20", 0.0003f, 0.0258f, 0.01861f, 0.544331f, 0.25f, 31.0f, 20},
         {"weapon_g3sg1", 0.0003f, 0.0258f, 0.01861f, 0.544331f, 0.25f, 30.0f, 20}}};
    inline const SniperProfile *sniperProfile(std::string_view name) {
        for (const auto &profile : sniperProfiles)
            if (profile.name == name)
                return &profile;
        return nullptr;
    }
} // namespace crosshair
