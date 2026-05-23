#ifndef BATTLEGRID_SENSE_INDICATOR_HPP_INCLUDED
#define BATTLEGRID_SENSE_INDICATOR_HPP_INCLUDED

#include "defines.hpp"
#include "libsim/types.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace battlegrid {

/// A floating visual indicator that appears when an entity uses a sense.
struct SenseIndicator {
    COORD    position;  ///< World position of the source entity
    grid::libsim::Senses sense;
    float    lifetime;  ///< Seconds remaining (starts at maxLifetime)
    float    maxLifetime;

    float alpha() const { return lifetime / maxLifetime; } // fade out
};

/// A directional ping on the HUD edge when the player perceives something.
struct HudPing {
    float    bearing;   ///< Angle in radians relative to camera yaw (0 = forward)
    grid::libsim::Senses sense;
    float    lifetime;
    float    maxLifetime;

    float alpha() const { return lifetime / maxLifetime; }
};

/// Color mapping for senses (r, g, b).
inline void senseColor(grid::libsim::Senses sense, float& r, float& g, float& b)
{
    using S = grid::libsim::Senses;
    switch (sense) {
    case S::Hearing: r = 1.0f; g = 0.85f; b = 0.1f;  break; // yellow
    case S::Sight:   r = 0.2f; g = 0.9f;  b = 1.0f;  break; // cyan
    case S::Smell:   r = 0.3f; g = 0.9f;  b = 0.3f;  break; // green
    case S::Touch:   r = 1.0f; g = 0.5f;  b = 0.1f;  break; // orange
    case S::Taste:   r = 0.9f; g = 0.3f;  b = 0.7f;  break; // pink
    default:         r = 1.0f; g = 1.0f;  b = 1.0f;  break; // white
    }
}

/// Manages the collection of active indicators and HUD pings.
class SenseIndicatorManager {
public:
    static constexpr float kIndicatorDuration = 2.5f;
    static constexpr float kHudPingDuration   = 2.0f;

    /// Add a 3D floating indicator above a world position.
    void addIndicator(const COORD& position, grid::libsim::Senses sense)
    {
        m_indicators.push_back({position, sense, kIndicatorDuration, kIndicatorDuration});
    }

    /// Add a directional HUD ping for the player.
    void addHudPing(float bearing, grid::libsim::Senses sense)
    {
        m_hudPings.push_back({bearing, sense, kHudPingDuration, kHudPingDuration});
    }

    /// Tick down lifetimes and remove expired entries.
    void update(float dt)
    {
        for (auto& ind : m_indicators) ind.lifetime -= dt;
        for (auto& hp  : m_hudPings)   hp.lifetime  -= dt;

        m_indicators.erase(
            std::remove_if(m_indicators.begin(), m_indicators.end(),
                           [](const SenseIndicator& i) { return i.lifetime <= 0.0f; }),
            m_indicators.end());
        m_hudPings.erase(
            std::remove_if(m_hudPings.begin(), m_hudPings.end(),
                           [](const HudPing& h) { return h.lifetime <= 0.0f; }),
            m_hudPings.end());
    }

    const std::vector<SenseIndicator>& indicators() const { return m_indicators; }
    const std::vector<HudPing>&        hudPings()   const { return m_hudPings; }

private:
    std::vector<SenseIndicator> m_indicators;
    std::vector<HudPing>        m_hudPings;
};

} // namespace battlegrid

#endif // BATTLEGRID_SENSE_INDICATOR_HPP_INCLUDED
