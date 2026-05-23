#ifndef GRID_SENSE_EVENT_HPP_INCLUDED
#define GRID_SENSE_EVENT_HPP_INCLUDED

#include "libevent/event.hpp"
#include "libsim/types.hpp"

#include <cmath>
#include <string>

namespace grid::libsim {

    /**
     * @brief Base class for spatially-attenuated sense events.
     *
     * A SenseEvent represents something perceptible in the world — a
     * sound, a visual, a smell, etc.  It has a point of origin, an
     * emission shape (who can perceive it?), and an intensity that
     * falls off with distance.
     *
     * Subclasses override contains() and attenuate() to define the
     * emission geometry and falloff curve.  The engine's emitSenseEvent()
     * delivers a per-receiver copy with perceivedIntensity filled in.
     *
     * @tparam NUMBER_TYPE  Coordinate scalar type (int, float, …)
     * @tparam NUM_DIMENSIONS  Number of spatial dimensions
     */
    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    class SenseEvent : public grid::libevent::Event {
    public:
        using Coord = VectX<NUMBER_TYPE, NUM_DIMENSIONS>;

        SenseEvent(const std::string& eventKey,
                   const std::string& source,
                   const Coord& origin,
                   Senses sense,
                   float intensity,
                   float range,
                   const std::string& message)
            : Event(eventKey)
            , m_source(source)
            , m_origin(origin)
            , m_sense(sense)
            , m_intensity(intensity)
            , m_range(range)
            , m_perceivedIntensity(0.0f)
            , m_message(message)
        {}

        // -- Accessors -------------------------------------------------------

        const std::string& source()  const { return m_source; }
        const Coord&       origin()  const { return m_origin; }
        Senses             sense()   const { return m_sense; }
        float              intensity() const { return m_intensity; }
        float              range()   const { return m_range; }
        const std::string& message() const { return m_message; }

        float perceivedIntensity() const { return m_perceivedIntensity; }
        void  setPerceivedIntensity(float v) { m_perceivedIntensity = v; }

        // -- Spatial queries (override in subclasses) -------------------------

        /// Does the emission region contain @p pos?
        /// Default implementation: radial check (circle / sphere).
        virtual bool contains(const Coord& pos) const
        {
            return distanceTo(pos) <= m_range;
        }

        /// Compute perceived intensity at @p distance from the origin.
        /// Default: linear falloff from full intensity to zero at range.
        virtual float attenuate(float distance) const
        {
            if (distance >= m_range) return 0.0f;
            if (m_range <= 0.0f)     return m_intensity;
            return m_intensity * (1.0f - distance / m_range);
        }

        // -- Helpers ----------------------------------------------------------

        /// Euclidean distance from origin to @p pos (as float).
        float distanceTo(const Coord& pos) const
        {
            Coord diff = pos - m_origin;
            // Use float arithmetic regardless of NUMBER_TYPE
            float sumSq = 0.0f;
            for (size_t i = 0; i < NUM_DIMENSIONS; ++i) {
                float d = static_cast<float>(diff[i]);
                sumSq += d * d;
            }
            return std::sqrt(sumSq);
        }

        std::unique_ptr<grid::libevent::Event> clone() const override
        {
            // Subclasses should override clone() for their own type.
            return std::make_unique<SenseEvent>(*this);
        }

    protected:
        std::string m_source;
        Coord       m_origin;
        Senses      m_sense;
        float       m_intensity;
        float       m_range;
        float       m_perceivedIntensity;
        std::string m_message;
    };

    // =====================================================================
    // AudioEvent — radial sound emission
    // =====================================================================

    /**
     * @brief A sound emitted from a point, attenuating over distance.
     *
     * Emission shape: circle / sphere (radial).
     * Falloff: linear by default (override attenuate() for inverse-square,
     * logarithmic, etc.).
     *
     * Future properties: pitch, direction, obstruction, …
     */
    template<typename NUMBER_TYPE, size_t NUM_DIMENSIONS>
    class AudioEvent : public SenseEvent<NUMBER_TYPE, NUM_DIMENSIONS> {
    public:
        using Coord = VectX<NUMBER_TYPE, NUM_DIMENSIONS>;
        using Base  = SenseEvent<NUMBER_TYPE, NUM_DIMENSIONS>;

        AudioEvent(const std::string& source,
                   const Coord& origin,
                   float loudness,
                   float range,
                   const std::string& message)
            : Base("sense.audio", source, origin,
                   Senses::Hearing, loudness, range, message)
        {}

        std::unique_ptr<grid::libevent::Event> clone() const override
        {
            return std::make_unique<AudioEvent>(*this);
        }

        // Inherits radial contains() and linear attenuate() from SenseEvent.
        // Override here for custom audio falloff in the future.
    };

} // namespace grid::libsim

#endif // GRID_SENSE_EVENT_HPP_INCLUDED
