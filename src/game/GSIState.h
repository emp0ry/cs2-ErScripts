#pragma once

#include <nlohmann/json_fwd.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

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
        bool operator==(const GameState &) const = default;
    };

    class Tracker {
        PlayerState state;
        GameState game;
        std::string mapName, phase;
        int round = -1;
        std::uint64_t sessionCounter = 0;
        std::optional<std::uint64_t> providerTimestamp;
        std::string providerID;

      public:
        PlayerState snapshot(Clock::time_point now = Clock::now()) const {
            return state.playing && now - state.received <= staleAfter ? state : PlayerState{};
        }

        GameState gameSnapshot(Clock::time_point now = Clock::now()) const {
            return game.connected && now - state.received <= staleAfter ? game : GameState{};
        }

        bool update(const nlohmann::json &payload, Clock::time_point now = Clock::now());
    };
} // namespace gsi
