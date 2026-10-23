#pragma once
#include "CrosshairGeometry.h"
#include "GSIPlayerState.h"
#include "SniperProfiles.h"

namespace crosshair {
    inline float compressedSpread(float value, float limit) {
        const float threshold = .75f * limit;
        return value <= threshold ? value
                                  : limit - std::exp(-(value - threshold) / (limit - threshold)) * (limit - threshold);
    }

    class SniperDynamics {
        const SniperProfile *profile = nullptr;
        std::uint64_t session = 0, shots = 0;
        gsi::Clock::time_point updated{}, received{};
        float penalty = 0, feedback = 0;

        void advance(gsi::Clock::time_point now) {
            const float elapsed = std::max(0.f, std::chrono::duration<float>(now - updated).count());
            penalty *= std::exp(-std::log(10.f) * elapsed / profile->recovery);
            feedback = std::max(0.f, feedback - 42.f * elapsed);
            updated = now;
        }

      public:
        void update(const gsi::PlayerState &player, gsi::Clock::time_point now = gsi::Clock::now()) {
            const auto nextProfile = player.sniper() ? sniperProfile(player.weapon) : nullptr;
            if (!nextProfile) {
                *this = {};
                return;
            }
            if (player.reloading || profile != nextProfile || session != player.weaponSession ||
                player.confirmedShots < shots || now - received > gsi::staleAfter) {
                profile = nextProfile;
                session = player.weaponSession;
                shots = player.confirmedShots;
                updated = received = now;
                penalty = feedback = 0;
                return;
            }
            const auto count = player.confirmedShots - shots;
            if (count > profile->magazine) {
                penalty = feedback = 0;
                updated = now;
            } else if (count > 0) {
                const float elapsed = std::max(0.f, std::chrono::duration<float>(now - updated).count());
                const float spacing = std::min(profile->cycle, elapsed / static_cast<float>(count));
                for (std::uint64_t i = 0; i < count; ++i) {
                    const auto age = std::chrono::duration<float>(spacing * static_cast<float>(count - i - 1));
                    const auto shot = now - std::chrono::duration_cast<gsi::Clock::duration>(age);
                    advance(shot);
                    penalty = std::min(1.f - profile->standing, penalty + profile->fire);
                    feedback = std::min(25.f, feedback + std::trunc(profile->recoil) / 3.5f);
                }
            }
            advance(now);
            received = now;
            shots = player.confirmedShots;
        }

        Frame frame(const Settings &settings, int height, gsi::Clock::time_point now = gsi::Clock::now()) const {
            Frame result;
            if (!profile || height <= 0 || now - received > gsi::staleAfter)
                return result;
            const float elapsed = std::max(0.f, std::chrono::duration<float>(now - updated).count());
            const float inaccuracy =
                std::min(1.f, profile->standing + penalty * std::exp(-std::log(10.f) * elapsed / profile->recovery));
            const float unscaled = (inaccuracy + profile->spread) * 320.f;
            const float projected = std::round(unscaled * static_cast<float>(height) / 480.f);
            const auto d = dimensions(settings, height);
            const float limit = static_cast<float>(d.spreadLimit) + 64.f;
            result.spread = compressedSpread(projected, limit);
            result.quadCircle = projected < 100.f;
            result.legacyDistance = projected;
            result.legacyMinimumDistance =
                std::round(std::min(unscaled, static_cast<float>(d.splitDistance)) * height / 480.f);
            result.legacySplit = unscaled > d.splitDistance;
            result.shotFeedback = std::max(0.f, feedback - 42.f * elapsed);
            return result;
        }
    };
} // namespace crosshair
