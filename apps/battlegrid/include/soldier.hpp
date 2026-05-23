#ifndef BATTLEGRID_SOLDIER_HPP_INCLUDED
#define BATTLEGRID_SOLDIER_HPP_INCLUDED

#include "defines.hpp"
#include "entity_types.hpp"
#include "terrain.hpp"

#include "libphysics/kinematic_body.hpp"
#include "libsim/base_agent.hpp"
#include "libsim/sense_event.hpp"

namespace battlegrid {

/**
 * @brief A soldier agent that can move on land and mountains.
 *
 * Soldiers patrol between waypoints, respond to audio events,
 * and can be player-controlled.
 */
class Soldier : public BASE_AGENT {
public:
    Soldier(I_ENVIRONMENT& env,
            const COORD& location,
            const std::string& name,
            Faction faction,
            const TerrainMap& map,
            double speed = 5.0,
            float audioRange = 30.0f);

    void start() override;
    void update(grid::libsim::DeltaType delta) override;
    void on_event(grid::libevent::Event* event) override;

    Faction     faction()    const { return m_faction; }
    EntityType  entityType() const { return EntityType::Soldier; }
    double      speed()      const { return m_speed; }

    void setSpeedMultiplier(double mul) { m_speedMultiplier = mul; }
    double speedMultiplier() const { return m_speedMultiplier; }

    /// Mark as player-controlled — movement handled on the render thread.
    void setPlayerControlled(bool pc) { m_playerControlled = pc; }
    bool isPlayerControlled() const { return m_playerControlled; }

    /// Disable local autonomous updates when the actor is driven externally.
    void setRemoteOwned(bool r) { m_remoteOwned = r; }
    bool isRemoteOwned() const { return m_remoteOwned; }

    /// Set a movement target in world coordinates.
    void setMoveTarget(const COORD& target);
    void clearMoveTarget();
    bool hasMoveTarget() const { return m_hasTarget; }

    /// Set facing direction (yaw in radians, 0 = +X).
    void setYaw(double yaw) { m_yaw = yaw; }
    double yaw() const { return m_yaw; }

    /// Jump (sets vertical velocity if grounded).
    void jump();
    bool isGrounded() const;

    grid::physics::KinematicBody& body() { return m_body; }
    const grid::physics::KinematicBody& body() const { return m_body; }

    double health() const { return m_health; }
    double maxHealth() const { return kMaxHealth; }
    bool   isDead() const { return m_health <= 0.0; }
    void   takeDamage(double amount);
    void   setHealth(double h) { m_health = h; }

    static constexpr double kCollisionRadius =  0.4;
    static constexpr double kMass            = 80.0; // kg
    static constexpr double kMaxHealth       = 100.0;
    static constexpr double kDamageThreshold = 10.0; // impulse/mass below this is harmless

private:
    void moveTowardTarget(double dt);
    void applyGravity(double dt);

    Faction           m_faction;
    const TerrainMap& m_map;
    double            m_speed;
    double            m_speedMultiplier = 1.0;
    float             m_audioRange;
    bool              m_playerControlled = false;
    bool              m_remoteOwned = false;
    double            m_yaw = 0.0;
    double            m_health = kMaxHealth;

    bool   m_hasTarget = false;
    COORD  m_target{0.0, 0.0, 0.0};
    grid::physics::KinematicBody m_body;

    static constexpr double kGravity         = 20.0;
    static constexpr double kJumpSpeed       =  8.0;
    static constexpr double kMaxStepUp       =  1.2;
};

} // namespace battlegrid

#endif // BATTLEGRID_SOLDIER_HPP_INCLUDED
