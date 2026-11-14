#pragma once
#include "game/gsi_state.hpp"
#include "graphics/crosshair_geometry.hpp"
#include "graphics/sniper_profiles.hpp"

namespace crosshair {
    inline float compressedSpread(float value, float limit) {
        const float threshold = .75f * limit;
        return value <= threshold ? value
                                  : limit - std::exp(-(value - threshold) / (limit - threshold)) * (limit - threshold);
    }

    class SniperDynamics {
        const SniperProfile* profile_ = nullptr;
        std::uint64_t session_ = 0, shots_ = 0;
        gsi::Clock::time_point updated_{}, received_{};
        float penalty_ = 0, feedback_ = 0;

        void advance(gsi::Clock::time_point now) {
            const float elapsed = std::max(0.f, std::chrono::duration<float>(now - updated_).count());
            penalty_ *= std::exp(-std::log(10.f) * elapsed / profile_->recovery);
            feedback_ = std::max(0.f, feedback_ - 42.f * elapsed);
            updated_ = now;
        }

      public:
        void update(const gsi::PlayerState& player, gsi::Clock::time_point now = gsi::Clock::now()) {
            const auto nextProfile = player.sniper() ? sniperProfile(player.weapon) : nullptr;
            if (!nextProfile) {
                *this = {};
                return;
            }
            if (player.reloading || profile_ != nextProfile || session_ != player.weaponSession ||
                player.confirmedShots < shots_ || now - received_ > gsi::staleAfter) {
                profile_ = nextProfile;
                session_ = player.weaponSession;
                shots_ = player.confirmedShots;
                updated_ = received_ = now;
                penalty_ = feedback_ = 0;
                return;
            }
            const auto count = player.confirmedShots - shots_;
            if (count > profile_->magazine) {
                penalty_ = feedback_ = 0;
                updated_ = now;
            } else if (count > 0) {
                const float elapsed = std::max(0.f, std::chrono::duration<float>(now - updated_).count());
                const float spacing = std::min(profile_->cycle, elapsed / static_cast<float>(count));
                for (std::uint64_t i = 0; i < count; ++i) {
                    const auto age = std::chrono::duration<float>(spacing * static_cast<float>(count - i - 1));
                    const auto shot = now - std::chrono::duration_cast<gsi::Clock::duration>(age);
                    advance(shot);
                    penalty_ = std::min(1.f - profile_->standing, penalty_ + profile_->fire);
                    feedback_ = std::min(25.f, feedback_ + std::trunc(profile_->recoil) / 3.5f);
                }
            }
            advance(now);
            received_ = now;
            shots_ = player.confirmedShots;
        }

        Frame frame(const Settings& settings, int height, gsi::Clock::time_point now = gsi::Clock::now()) const {
            Frame result;
            if (!profile_ || height <= 0 || now - received_ > gsi::staleAfter)
                return result;
            const float elapsed = std::max(0.f, std::chrono::duration<float>(now - updated_).count());
            const float inaccuracy =
                std::min(1.f, profile_->standing + penalty_ * std::exp(-std::log(10.f) * elapsed / profile_->recovery));
            const float unscaled = (inaccuracy + profile_->spread) * 320.f;
            const float projected = std::round(unscaled * static_cast<float>(height) / 480.f);
            const auto d = dimensions(settings, height);
            const float limit = static_cast<float>(d.spreadLimit) + 64.f;
            result.spread = compressedSpread(projected, limit);
            result.quadCircle = projected < 100.f;
            result.legacyDistance = projected;
            result.legacyMinimumDistance =
                std::round(std::min(unscaled, static_cast<float>(d.splitDistance)) * height / 480.f);
            result.legacySplit = unscaled > d.splitDistance;
            result.shotFeedback = std::max(0.f, feedback_ - 42.f * elapsed);
            return result;
        }
    };
} // namespace crosshair
