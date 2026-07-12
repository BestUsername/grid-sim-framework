#include "vehicle.hpp"
#include "soldier.hpp"

#include "libphysics/collision_event.hpp"

#include <algorithm>
#include <cmath>

namespace battlegrid {

// ── Vehicle (base) ──────────────────────────────────────────────────

Vehicle::Vehicle(I_ENVIRONMENT& env,
                 const COORD& location,
                 const std::string& name,
                 Faction faction,
                 EntityType vehicleType,
                 const TerrainMap& map,
                 double speed)
    : BASE_AGENT(env, location, name)
    , m_faction(faction)
    , m_vehicleType(vehicleType)
    , m_speed(speed)
{
}

void Vehicle::start()
{
    communicate(grid::libsim::Senses::Hearing,
                entityTypeName(m_vehicleType) + " " + m_name + " engine started.");
}

void Vehicle::update(grid::libsim::DeltaType delta)
{
    BASE_AGENT::update(delta);
    if (m_remoteOwned) return;

    if (isDead()) {
        // The world keeps a mounted driver attached until it transfers the
        // driver back to an independent collision body.
        m_movementVelocity = {0.0, 0.0};
        return;
    }

    // Submit intention only. BattleGridWorld applies Box3D's solved
    // transform to this vehicle and any mounted driver.
    double dt = delta.count();
    if (m_hasTarget) {
        moveTowardTarget(dt);
    }
}

void Vehicle::takeDamage(double amount)
{
    if (isDead()) return;
    m_health -= amount;
    if (m_health <= 0.0) {
        m_health = 0.0;
        m_hasTarget = false;
        m_environment.getGameLog().log(
            m_name, "", grid::libsim::Senses::Sight,
            m_name + " has been destroyed!");
    }
}

void Vehicle::on_event(grid::libevent::Event* event)
{
    if (isDead()) return;

    auto* col = dynamic_cast<grid::physics::CollisionEvent*>(event);
    if (!col) return;

    m_environment.getGameLog().log(
        m_name, "", grid::libsim::Senses::Touch,
        "Impact with " + col->otherEntity() + "!");
    double force = col->impulse() / col->thisMass();
    if (force > kDamageThreshold) {
        takeDamage(force - kDamageThreshold);
    }
}

void Vehicle::setMoveTarget(const COORD& target)
{
    m_target    = target;
    m_hasTarget = true;
}

void Vehicle::clearMoveTarget()
{
    m_hasTarget = false;
    m_movementVelocity = {0.0, 0.0};
}

bool Vehicle::mount(Soldier* s)
{
    if (m_driver || !s) return false;
    m_driver = s;
    m_driver->setYaw(m_yaw);
    communicate(grid::libsim::Senses::Hearing,
                s->name() + " boards " + m_name + ".");
    return true;
}

void Vehicle::dismount()
{
    if (!m_driver) return;
    communicate(grid::libsim::Senses::Hearing,
                m_driver->name() + " exits " + m_name + ".");
    m_driver = nullptr;
    clearMoveTarget();
}

void Vehicle::moveTowardTarget(double dt)
{
    COORD diff = m_target - m_location;
    double dx = diff[0];
    double dz = diff[2];
    double dist = std::sqrt(dx * dx + dz * dz);

    if (dist < 0.5) {
        clearMoveTarget();
        return;
    }

    double nx = dx / dist;
    double nz = dz / dist;
    double effectiveSpeed = std::min(m_speed, dist / std::max(dt, 1e-12));
    m_movementVelocity = {nx * effectiveSpeed, nz * effectiveSpeed};
    setYaw(std::atan2(nz, nx));
}

// ── LandVehicle ─────────────────────────────────────────────────────

LandVehicle::LandVehicle(I_ENVIRONMENT& env, const COORD& location,
                         const std::string& name, Faction faction,
                         const TerrainMap& map, double speed)
    : Vehicle(env, location, name, faction, EntityType::LandVehicle, map, speed)
{
}

// ── SeaVehicle ──────────────────────────────────────────────────────

SeaVehicle::SeaVehicle(I_ENVIRONMENT& env, const COORD& location,
                       const std::string& name, Faction faction,
                       const TerrainMap& map, double speed)
    : Vehicle(env, location, name, faction, EntityType::SeaVehicle, map, speed)
{
}

// ── AirVehicle ──────────────────────────────────────────────────────

AirVehicle::AirVehicle(I_ENVIRONMENT& env, const COORD& location,
                       const std::string& name, Faction faction,
                       const TerrainMap& map, double speed,
                       double altitude)
    : Vehicle(env, COORD{location[0], location[1] + altitude, location[2]},
              name, faction, EntityType::AirVehicle, map, speed)
    , m_cruiseHeight(location[1] + altitude)
{
}

void AirVehicle::update(grid::libsim::DeltaType delta)
{
    Vehicle::update(delta);
}

grid::physics::Vec3 AirVehicle::movementVelocity() const
{
    auto velocity = Vehicle::movementVelocity();
    velocity[1] = std::clamp((m_cruiseHeight - m_location[1]) * 4.0, -m_speed, m_speed);
    return velocity;
}

} // namespace battlegrid
