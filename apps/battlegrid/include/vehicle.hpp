#ifndef BATTLEGRID_VEHICLE_HPP_INCLUDED
#define BATTLEGRID_VEHICLE_HPP_INCLUDED

#include "defines.hpp"
#include "entity_types.hpp"
#include "terrain.hpp"

#include "libphysics/kinematic_body.hpp"
#include "libsim/base_agent.hpp"

namespace battlegrid {

class Soldier; // forward

// ── Terrain collision outcomes ──────────────────────────────────────

/// What happens when a vehicle hits a terrain height step.
enum class TerrainCollisionOutcome {
    Pass,          ///< Within normal step-up — proceed unimpeded.
    SpeedBump,     ///< Small bump — vehicle passes, takes speed-based damage.
    CrashThrough,  ///< Enough momentum to break through — passes with damage.
    HardStop,      ///< Cannot break through — full stop + impact damage.
};

/// Result of evaluating a terrain collision.
struct TerrainCollisionResult {
    TerrainCollisionOutcome outcome = TerrainCollisionOutcome::Pass;
    double damage          = 0.0;  ///< Damage dealt to the vehicle.
    double speedMultiplier = 1.0;  ///< Applied to movement (0 = stop, 1 = full).
};

/// Evaluate what happens when a vehicle encounters a terrain height step.
/// @param heightDelta       targetGroundH − currentFootY (positive = uphill).
/// @param maxStepUp         vehicle's normal step-up tolerance.
/// @param vehicleSpeed      current effective speed.
/// @param vehicleMass       mass of the vehicle.
/// @param obstacleStrength  terrain's resistance (terrainObstacleStrength).
inline TerrainCollisionResult resolveTerrainCollision(
        double heightDelta, double maxStepUp,
        double vehicleSpeed, double vehicleMass,
        double obstacleStrength)
{
    // Downhill or within normal step-up — no collision.
    if (heightDelta <= maxStepUp)
        return {TerrainCollisionOutcome::Pass, 0.0, 1.0};

    double excess = heightDelta - maxStepUp;

    // Small bump: excess within half the step-up tolerance.
    if (excess <= maxStepUp * 0.5) {
        double damage = vehicleSpeed * excess * 0.5;
        return {TerrainCollisionOutcome::SpeedBump, damage, 0.7};
    }

    // Larger obstacle — compare momentum against terrain strength.
    double momentum = vehicleSpeed * vehicleMass;
    if (obstacleStrength > 0.0 && momentum > obstacleStrength) {
        double damage = obstacleStrength / vehicleMass;
        return {TerrainCollisionOutcome::CrashThrough, damage, 0.3};
    }

    // Can't break through — hard stop.
    double damage = vehicleSpeed * 2.0;
    return {TerrainCollisionOutcome::HardStop, damage, 0.0};
}

/**
 * @brief Base class for all vehicles in the simulation.
 */
class Vehicle : public BASE_AGENT {
public:
    Vehicle(I_ENVIRONMENT& env,
            const COORD& location,
            const std::string& name,
            Faction faction,
            EntityType vehicleType,
            const TerrainMap& map,
            double speed);

    void start() override;
    void update(grid::libsim::DeltaType delta) override;
    void on_event(grid::libevent::Event* event) override;

    Faction     faction()     const { return m_faction; }
    EntityType  entityType()  const { return m_vehicleType; }
    double      speed()       const { return m_speed; }
    double      yaw()         const { return m_yaw; }
    void        setYaw(double yaw) { m_yaw = yaw; }

    void setMoveTarget(const COORD& target);
    void clearMoveTarget();
    bool hasMoveTarget() const { return m_hasTarget; }

    /// Disable local autonomous updates when the actor is driven externally.
    void setRemoteOwned(bool r) { m_remoteOwned = r; }
    bool isRemoteOwned() const { return m_remoteOwned; }

    /// Occupant (driver) management
    bool hasDriver() const { return m_driver != nullptr; }
    Soldier* driver() const { return m_driver; }
    bool mount(Soldier* s);
    void dismount();

    double health() const { return m_health; }
    double maxHealth() const { return kMaxHealth; }
    bool   isDead() const { return m_health <= 0.0; }
    void   takeDamage(double amount);
    void   setHealth(double h) { m_health = h; }

    static constexpr double kCollisionRadius = 1.0;
    static constexpr double kMass            = 2000.0; // kg
    static constexpr double kMaxHealth       = 500.0;
    static constexpr double kDamageThreshold = 50.0;
    static constexpr double kMaxStepUp       = 2.0;

protected:
    virtual bool canTraverse(TerrainType t) const = 0;
    void moveTowardTarget(double dt);
    void applyGravity(double dt);

    Faction           m_faction;
    EntityType        m_vehicleType;
    const TerrainMap& m_map;
    double            m_speed;
    bool              m_hasTarget = false;
    COORD             m_target{0.0, 0.0, 0.0};
    double            m_yaw = 0.0;
    bool              m_remoteOwned = false;
    Soldier*          m_driver = nullptr;
    double            m_health = kMaxHealth;
    grid::physics::KinematicBody m_body;

    static constexpr double kGravity         = 20.0;
};

// ── Concrete vehicle types ──────────────────────────────────────────

class LandVehicle : public Vehicle {
public:
    LandVehicle(I_ENVIRONMENT& env, const COORD& location,
                const std::string& name, Faction faction,
                const TerrainMap& map, double speed = 10.0);

protected:
    bool canTraverse(TerrainType t) const override { return isTraversableByLand(t); }
};

class SeaVehicle : public Vehicle {
public:
    SeaVehicle(I_ENVIRONMENT& env, const COORD& location,
               const std::string& name, Faction faction,
               const TerrainMap& map, double speed = 12.0);

protected:
    bool canTraverse(TerrainType t) const override { return isTraversableBySea(t); }
};

class AirVehicle : public Vehicle {
public:
    AirVehicle(I_ENVIRONMENT& env, const COORD& location,
               const std::string& name, Faction faction,
               const TerrainMap& map, double speed = 20.0,
               double altitude = 15.0);

    void update(grid::libsim::DeltaType delta) override;

protected:
    bool canTraverse(TerrainType /*t*/) const override { return true; }

private:
    double m_altitude;
};

} // namespace battlegrid

#endif // BATTLEGRID_VEHICLE_HPP_INCLUDED
