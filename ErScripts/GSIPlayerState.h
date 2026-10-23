#pragma once

#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace gsi {
    using Clock = std::chrono::steady_clock;
    constexpr auto staleAfter = std::chrono::seconds(25);

    inline std::string text(const nlohmann::json &object, const char *key) {
        const auto value = object.find(key);
        return value != object.end() && value->is_string() ? value->get<std::string>() : std::string{};
    }

    inline int count(const nlohmann::json &object, const char *key) {
        const auto value = object.find(key);
        if (value == object.end() || !value->is_number_integer() || *value < 0 || *value > 65535)
            return -1;
        return value->get<int>();
    }

    inline const nlohmann::json *localPlayer(const nlohmann::json &payload) {
        const auto provider = payload.find("provider"), player = payload.find("player");
        if (provider == payload.end() || player == payload.end() || !provider->is_object() || !player->is_object())
            return nullptr;
        const auto id = text(*provider, "steamid");
        if (id.empty() || text(*player, "steamid") != id)
            return nullptr;
        const auto appid = provider->find("appid");
        if (appid != provider->end() && (!appid->is_number_integer() || *appid != 730))
            return nullptr;
        return &*player;
    }

    inline bool isPlaying(const nlohmann::json &player) {
        const auto state = player.find("state");
        const auto team = text(player, "team");
        return text(player, "activity") == "playing" && (team == "T" || team == "CT") && state != player.end() &&
               state->is_object() && count(*state, "health") > 0;
    }

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

    class AmmoTracker {
        PlayerState state;
        std::string mapName, phase;
        int round = -1;
        std::uint64_t sessionCounter = 0;
        std::optional<std::uint64_t> providerTimestamp;

      public:
        PlayerState snapshot(Clock::time_point now = Clock::now()) const {
            return state.playing && now - state.received <= staleAfter ? state : PlayerState{};
        }

        void update(const nlohmann::json &payload, Clock::time_point now = Clock::now()) {
            const auto provider = payload.find("provider");
            std::optional<std::uint64_t> timestamp;
            if (provider != payload.end() && provider->is_object()) {
                const auto value = provider->find("timestamp");
                if (value != provider->end() && value->is_number_integer() && *value >= 0)
                    timestamp = value->get<std::uint64_t>();
            }
            if (timestamp && providerTimestamp && *timestamp < *providerTimestamp)
                return;
            providerTimestamp = timestamp;

            const auto player = localPlayer(payload);
            PlayerState next;
            next.received = now;
            if (player && isPlaying(*player)) {
                next.playing = true;
                next.steamid = text(*player, "steamid");
                const auto weapons = player->find("weapons");
                if (weapons != player->end() && weapons->is_object()) {
                    for (const auto &[slot, weapon] : weapons->items()) {
                        const auto status = text(weapon, "state");
                        if (!weapon.is_object() || (status != "active" && status != "reloading"))
                            continue;
                        if (!next.slot.empty()) {
                            next.slot.clear();
                            next.weapon.clear();
                            next.type.clear();
                            next.clip = next.reserve = -1;
                            break;
                        }
                        next.slot = slot;
                        next.weapon = text(weapon, "name");
                        next.type = text(weapon, "type");
                        next.reloading = status == "reloading";
                        next.clip = count(weapon, "ammo_clip");
                        next.reserve = count(weapon, "ammo_reserve");
                        const int maximum = count(weapon, "ammo_clip_max");
                        if (maximum >= 0 && next.clip > maximum)
                            next.clip = -1;
                    }
                }
            }

            const auto map = payload.find("map"), roundData = payload.find("round");
            const auto nextMap = map != payload.end() ? text(*map, "name") : std::string{};
            const int nextRound = map != payload.end() ? count(*map, "round") : -1;
            const auto nextPhase = roundData != payload.end() ? text(*roundData, "phase") : std::string{};
            const bool continuous = next.playing && state.playing && next.steamid == state.steamid &&
                                    !next.weapon.empty() && !next.slot.empty() && next.weapon == state.weapon &&
                                    next.slot == state.slot && next.type == state.type && nextMap == mapName &&
                                    nextRound == round && !(nextPhase == "freezetime" && phase != nextPhase) &&
                                    now - state.received <= staleAfter;
            if (continuous) {
                next.weaponSession = state.weaponSession;
                next.confirmedShots = state.confirmedShots;
                next.lastShotReceipt = state.lastShotReceipt;
                if (!state.reloading && !next.reloading && state.clip >= 0 && next.clip >= 0 &&
                    next.clip < state.clip &&
                    (state.reserve < 0 || next.reserve < 0 || next.reserve <= state.reserve)) {
                    next.confirmedShots += static_cast<std::uint64_t>(state.clip - next.clip);
                    next.lastShotReceipt = now;
                }
            }
            if (!continuous && next.playing && !next.weapon.empty() && !next.slot.empty())
                next.weaponSession = ++sessionCounter;
            state = std::move(next);
            mapName = nextMap;
            round = nextRound;
            phase = nextPhase;
        }
    };
} // namespace gsi
