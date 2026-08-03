#ifndef BATTLEGRID_VEHICLE_HPP_INCLUDED
#define BATTLEGRID_VEHICLE_HPP_INCLUDED

#include "defines.hpp"
#include "entity_types.hpp"
#include "terrain.hpp"

#include "libphysics/collision_body.hpp"
#include "libsim/base_agent.hpp"

#include <array>

namespace battlegrid {

class Soldier; // forward

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
    double      desiredYaw()  const { return m_desiredYaw; }
    void        setYaw(double yaw) { m_desiredYaw = yaw; }
    void        setPhysicsYaw(double yaw) { m_yaw = yaw; }

    void setMoveTarget(const COORD& target);
    void clearMoveTarget();
    bool hasMoveTarget() const { return m_hasTarget; }
    virtual grid::physics::Vec3 movementVelocity() const
    {
        return {m_movementVelocity[0], 0.0, m_movementVelocity[1]};
    }
    virtual double gravityScale() const { return 1.0; }

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
    void moveTowardTarget(double dt);

    Faction           m_faction;
    EntityType        m_vehicleType;
    double            m_speed;
    bool              m_hasTarget = false;
    COORD             m_target{0.0, 0.0, 0.0};
    double            m_yaw = 0.0;
    double            m_desiredYaw = 0.0;
    bool              m_remoteOwned = false;
    Soldier*          m_driver = nullptr;
    double            m_health = kMaxHealth;
    std::array<double, 2> m_movementVelocity{0.0, 0.0};
};

// ── Concrete vehicle types ──────────────────────────────────────────

class LandVehicle : public Vehicle {
public:
    LandVehicle(I_ENVIRONMENT& env, const COORD& location,
                const std::string& name, Faction faction,
                const TerrainMap& map, double speed = 10.0);

    static constexpr std::size_t kWheelCount = 4;
    std::string wheelName(std::size_t index) const;

};

class SeaVehicle : public Vehicle {
public:
    SeaVehicle(I_ENVIRONMENT& env, const COORD& location,
               const std::string& name, Faction faction,
               const TerrainMap& map, double speed = 12.0);

};

class AirVehicle : public Vehicle {
public:
    AirVehicle(I_ENVIRONMENT& env, const COORD& location,
               const std::string& name, Faction faction,
               const TerrainMap& map, double speed = 20.0,
               double altitude = 15.0);

    void update(grid::libsim::DeltaType delta) override;
    grid::physics::Vec3 movementVelocity() const override;
    double gravityScale() const override { return 0.0; }

protected:
private:
    double m_cruiseHeight;
};

} // namespace battlegrid

#endif // BATTLEGRID_VEHICLE_HPP_INCLUDED
