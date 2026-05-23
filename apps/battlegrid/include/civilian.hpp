#ifndef BATTLEGRID_CIVILIAN_HPP_INCLUDED
#define BATTLEGRID_CIVILIAN_HPP_INCLUDED

#include "defines.hpp"
#include "entity_types.hpp"
#include "terrain.hpp"

#include "libphysics/kinematic_body.hpp"
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

    /// Jump (sets vertical velocity if grounded).
    void jump();
    bool isGrounded() const;

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
    void applyGravity(double dt);

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
    grid::physics::KinematicBody m_body;

    static constexpr double kGravity         = 20.0;
    static constexpr double kJumpSpeed       =  8.0;
    static constexpr double kMaxStepUp       =  1.2;
};

} // namespace battlegrid

#endif // BATTLEGRID_CIVILIAN_HPP_INCLUDED
