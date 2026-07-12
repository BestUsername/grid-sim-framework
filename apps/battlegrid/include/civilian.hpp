#ifndef BATTLEGRID_CIVILIAN_HPP_INCLUDED
#define BATTLEGRID_CIVILIAN_HPP_INCLUDED

#include "defines.hpp"
#include "entity_types.hpp"
#include "terrain.hpp"

#include "libphysics/collision_body.hpp"
#include "libsim/base_agent.hpp"

namespace battlegrid {

/**
 * @brief A civilian agent that wanders on land.
 *
 * Civilians move randomly between nearby land tiles, fleeing
 * from nearby combat sounds.
 */
class Civilian : public BASE_AGENT {
public:
    Civilian(I_ENVIRONMENT& env,
             const COORD& location,
             const std::string& name,
             const TerrainMap& map,
             double speed = 3.0);

    void start() override;
    void update(grid::libsim::DeltaType delta) override;
    void on_event(grid::libevent::Event* event) override;

    EntityType entityType() const { return EntityType::Civilian; }
    double yaw() const { return m_yaw; }
    void   setYaw(double yaw) { m_yaw = yaw; }
    void   setHealth(double h) { m_health = h; }

    /// Disable local autonomous updates when the actor is driven externally.
    void setRemoteOwned(bool r) { m_remoteOwned = r; }
    bool isRemoteOwned() const { return m_remoteOwned; }

    /// Request a physics-world jump on the next simulation step.
    void jump();
    bool consumeJumpRequest();

    grid::physics::Vec3 movementVelocity() const
    {
        return {m_movementVelocity[0], 0.0, m_movementVelocity[1]};
    }

    double health() const { return m_health; }
    double maxHealth() const { return kMaxHealth; }
    bool   isDead() const { return m_health <= 0.0; }
    void   takeDamage(double amount);

    static constexpr double kCollisionRadius =  0.4;
    static constexpr double kMass            = 70.0; // kg
    static constexpr double kMaxHealth       = 50.0;
    static constexpr double kDamageThreshold = 10.0;

private:
    void pickNewWanderTarget();
    void moveTowardTarget(double dt);

    const TerrainMap& m_map;
    double            m_speed;
    bool              m_hasTarget = false;
    COORD             m_target{0.0, 0.0, 0.0};
    double            m_wanderTimer = 0.0;
    double            m_wanderInterval = 4.0;
    double            m_yaw = 0.0;
    bool              m_remoteOwned = false;
    bool              m_fleeing = false;
    double            m_health = kMaxHealth;
    std::array<double, 2> m_movementVelocity{0.0, 0.0};
    bool m_jumpRequested = false;

    static constexpr double kJumpSpeed = 8.0;
};

} // namespace battlegrid

#endif // BATTLEGRID_CIVILIAN_HPP_INCLUDED
