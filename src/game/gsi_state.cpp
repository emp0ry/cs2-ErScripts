#include "game/gsi_state.hpp"

#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

namespace gsi {
    namespace {
        std::string_view text(const nlohmann::json& object, const char* key) {
            const auto value = object.find(key);
            return value != object.end() && value->is_string() ? value->get_ref<const std::string&>()
                                                               : std::string_view{};
        }

        int count(const nlohmann::json& object, const char* key) {
            const auto value = object.find(key);
            if (value == object.end() || !value->is_number_integer() || *value < 0 || *value > 65535)
                return -1;
            return value->get<int>();
        }

        const nlohmann::json* localPlayer(const nlohmann::json& payload) {
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

        bool isPlaying(const nlohmann::json& player) {
            const auto state = player.find("state");
            const auto team = text(player, "team");
            return text(player, "activity") == "playing" && (team == "T" || team == "CT") && state != player.end() &&
                   state->is_object() && count(*state, "health") > 0;
        }

    } // namespace

    bool Tracker::update(const nlohmann::json& payload, Clock::time_point now) {
        if (!payload.is_object())
            return false;
        const auto provider = payload.find("provider");
        if (provider == payload.end() || !provider->is_object())
            return false;
        const auto id = text(*provider, "steamid");
        const auto appid = provider->find("appid");
        if (id.empty() || (appid != provider->end() && (!appid->is_number_integer() || *appid != 730)))
            return false;
        std::optional<std::uint64_t> timestamp;
        const auto value = provider->find("timestamp");
        if (value != provider->end()) {
            if (!value->is_number_integer() || *value < 0)
                return false;
            timestamp = value->get<std::uint64_t>();
        }
        if (timestamp && providerTimestamp_ && id == providerId_ && now - state_.received <= staleAfter &&
            *timestamp < *providerTimestamp_)
            return false;
        const auto player = localPlayer(payload);
        PlayerState next;
        GameState nextGame;
        nextGame.connected = true;
        next.received = now;
        if (player) {
            nextGame.inGame = text(*player, "activity") == "playing";
            const auto stats = player->find("match_stats"), playerState = player->find("state");
            if (stats != player->end())
                nextGame.kills = count(*stats, "kills");
            if (playerState != player->end()) {
                const auto kit = playerState->find("defusekit");
                nextGame.defuseKit = kit != playerState->end() && kit->is_boolean() && kit->get<bool>();
            }
            next.playing = isPlaying(*player);
            nextGame.alive = next.playing;
            if (next.playing)
                next.steamid = text(*player, "steamid");
            const auto weapons = player->find("weapons");
            bool ambiguous = false;
            if (weapons != player->end() && weapons->is_object()) {
                for (const auto& [slot, weapon] : weapons->items()) {
                    const auto status = text(weapon, "state"), type = text(weapon, "type"), name = text(weapon, "name");
                    if (!status.empty() && type == "C4")
                        nextGame.bombInWeapons = true;
                    if (status == "active") {
                        nextGame.knife |= type == "Knife";
                        nextGame.pistol |= type == "Pistol";
                        nextGame.revolver |= name == "weapon_revolver";
                        nextGame.cz75 |= name == "weapon_cz75a";
                    }
                    if (!next.playing || ambiguous || (status != "active" && status != "reloading"))
                        continue;
                    if (!next.slot.empty()) {
                        next.slot.clear();
                        next.weapon.clear();
                        next.type.clear();
                        next.clip = next.reserve = -1;
                        ambiguous = true;
                        continue;
                    }
                    next.slot = slot;
                    next.weapon = name;
                    next.type = type;
                    next.reloading = status == "reloading";
                    next.clip = count(weapon, "ammo_clip");
                    next.reserve = count(weapon, "ammo_reserve");
                    const int maximum = count(weapon, "ammo_clip_max");
                    if (maximum >= 0 && next.clip > maximum)
                        next.clip = -1;
                }
            }
            if (ambiguous)
                nextGame.knife = nextGame.pistol = nextGame.revolver = nextGame.cz75 = false;
        }

        const auto map = payload.find("map"), roundData = payload.find("round");
        const auto nextMap = map != payload.end() ? text(*map, "name") : std::string_view{};
        const int nextRound = map != payload.end() ? count(*map, "round") : -1;
        const auto nextPhase = roundData != payload.end() ? text(*roundData, "phase") : std::string_view{};
        nextGame.roundLive = nextPhase == "live";
        nextGame.bombPlanted = roundData != payload.end() && text(*roundData, "bomb") == "planted";
        const bool newSession =
            !game_.connected || id != providerId_ || nextMap != mapName_ || now - state_.received > staleAfter;
        nextGame.session = game_.session + (newSession ? 1 : 0);
        const bool continuous = next.playing && state_.playing && next.steamid == state_.steamid &&
                                !next.weapon.empty() && !next.slot.empty() && next.weapon == state_.weapon &&
                                next.slot == state_.slot && next.type == state_.type && nextMap == mapName_ &&
                                nextRound == round_ && !(nextPhase == "freezetime" && phase_ != nextPhase) &&
                                now - state_.received <= staleAfter;
        if (continuous) {
            next.weaponSession = state_.weaponSession;
            next.confirmedShots = state_.confirmedShots;
            next.lastShotReceipt = state_.lastShotReceipt;
            if (!state_.reloading && !next.reloading && state_.clip >= 0 && next.clip >= 0 && next.clip < state_.clip &&
                (state_.reserve < 0 || next.reserve < 0 || next.reserve <= state_.reserve)) {
                next.confirmedShots += static_cast<std::uint64_t>(state_.clip - next.clip);
                next.lastShotReceipt = now;
            }
        }
        if (!continuous && next.playing && !next.weapon.empty() && !next.slot.empty())
            next.weaponSession = ++sessionCounter_;
        if (timestamp || id != providerId_ || now - state_.received > staleAfter)
            providerTimestamp_ = timestamp;
        providerId_ = id;
        game_ = nextGame;
        state_ = std::move(next);
        mapName_ = nextMap;
        round_ = nextRound;
        phase_ = nextPhase;
        return true;
    }
} // namespace gsi
