#ifndef GRID_PHYSICS_COLLISION_EVENT_HPP_INCLUDED
#define GRID_PHYSICS_COLLISION_EVENT_HPP_INCLUDED

#include "libphysics/collision_body.hpp"
#include "libevent/event.hpp"

#include <memory>
#include <string>

namespace grid::physics {

/**
 * @brief Event delivered to an entity involved in a collision.
 *
 * Carries the deterministic physics impulse computed by PhysicsWorld
 * plus enough metadata for the receiving entity to decide its own
 * gameplay response (knockback, damage, animation, sound, etc.).
 *
 * Each collision produces two CollisionEvents — one per participant —
 * with @c thisEntity / @c otherEntity swapped and the normal flipped.
 */
class CollisionEvent : public grid::libevent::Event {
public:
    CollisionEvent(const std::string& thisEntity,
                   const std::string& otherEntity,
                   const Vec3& normal,
                   const Vec3& relativeVelocity,
                   double impulse,
                   double thisMass,
                   double otherMass,
                   double penetration)
        : Event("physics.collision")
        , m_thisEntity(thisEntity)
        , m_otherEntity(otherEntity)
        , m_normal(normal)
        , m_relativeVelocity(relativeVelocity)
        , m_impulse(impulse)
        , m_thisMass(thisMass)
        , m_otherMass(otherMass)
        , m_penetration(penetration)
    {}

    const std::string& thisEntity()  const { return m_thisEntity; }
    const std::string& otherEntity() const { return m_otherEntity; }

    /// Contact normal pointing from this entity toward the other.
    const Vec3& normal() const { return m_normal; }

    /// Velocity of this entity relative to the other at impact.
    const Vec3& relativeVelocity() const { return m_relativeVelocity; }

    /// Scalar impulse magnitude (N·s).
    double impulse() const { return m_impulse; }

    double thisMass()  const { return m_thisMass; }
    double otherMass() const { return m_otherMass; }

    /// Overlap depth along the contact normal.
    double penetration() const { return m_penetration; }

    std::unique_ptr<grid::libevent::Event> clone() const override
    {
        return std::make_unique<CollisionEvent>(*this);
    }

private:
    std::string m_thisEntity;
    std::string m_otherEntity;
    Vec3 m_normal;
    Vec3 m_relativeVelocity;
    double m_impulse;
    double m_thisMass;
    double m_otherMass;
    double m_penetration;
};

} // namespace grid::physics

#endif // GRID_PHYSICS_COLLISION_EVENT_HPP_INCLUDED
