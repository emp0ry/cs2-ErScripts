#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace gsi {
    using Clock = std::chrono::steady_clock;
    constexpr auto staleAfter = std::chrono::seconds(25);

    struct PlayerState {
        bool playing = false;
        std::string steamid, slot, weapon, type;
        bool reloading = false;
        int clip = -1, reserve = -1;
        std::uint64_t confirmedShots = 0, weaponSession = 0;
        Clock::time_point received{};
        std::optional<Clock::time_point> lastShotReceipt;

        bool sniper() const { return playing && !slot.empty() && !weapon.empty() && type == "SniperRifle"; }
    };

    struct GameState {
        bool connected = false, inGame = false, alive = false;
        bool bombPlanted = false, defuseKit = false, roundLive = false;
        bool knife = false, pistol = false, revolver = false, cz75 = false, bombInWeapons = false;
        int kills = -1;
        std::uint64_t session = 0;
        bool operator==(const GameState&) const = default;
    };

    class Tracker {
        PlayerState state_;
        GameState game_;
        std::string mapName_, phase_;
        int round_ = -1;
        std::uint64_t sessionCounter_ = 0;
        std::optional<std::uint64_t> providerTimestamp_;
        std::string providerId_;

      public:
        PlayerState snapshot(Clock::time_point now = Clock::now()) const {
            return state_.playing && now - state_.received <= staleAfter ? state_ : PlayerState{};
        }

        GameState gameSnapshot(Clock::time_point now = Clock::now()) const {
            return game_.connected && now - state_.received <= staleAfter ? game_ : GameState{};
        }

        bool update(const nlohmann::json& payload, Clock::time_point now = Clock::now());
    };
} // namespace gsi
