#include "vehicle.hpp"
#include "soldier.hpp"

#include "libphysics/collision_event.hpp"

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
    , m_map(map)
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
        // Eject driver on death
        if (m_driver) dismount();
        applyGravity(delta.count());
        return;
    }

    // If a driver soldier is riding, keep them anchored to the vehicle
    double dt = delta.count();
    if (m_hasTarget) {
        moveTowardTarget(dt);
    }
    applyGravity(dt);

    // Anchor driver *after* movement so they stay in sync.
    if (m_driver) {
        m_driver->set_location(m_location);
        m_driver->setYaw(m_yaw);
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
}

bool Vehicle::mount(Soldier* s)
{
    if (m_driver || !s) return false;
    m_driver = s;
    m_driver->set_location(m_location);
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
        m_hasTarget = false;
        return;
    }

    // Check terrain traversal
    auto ix = static_cast<size_t>(std::max(0.0, m_location[0]));
    auto iz = static_cast<size_t>(std::max(0.0, m_location[2]));
    TerrainType terrain = m_map.at(ix, iz);

    double groundH = m_map.heightAt(m_location[0], m_location[2]);
    bool grounded = m_body.isGrounded(m_location[1], groundH);
    double speedFactor = grounded ? terrainSpeedFactor(terrain) : 1.0;

    if (grounded && !canTraverse(terrain)) {
        m_hasTarget = false;
        return;
    }

    double effectiveSpeed = m_speed * speedFactor;
    double step = effectiveSpeed * dt;
    if (step > dist) step = dist;

    double nx = dx / dist;
    double nz = dz / dist;

    double newX = m_location[0] + nx * step;
    double newZ = m_location[2] + nz * step;

    // Slope / obstacle check: sample the whole footprint, not just the centre.
    auto sample = m_map.maxTerrainInRadius(newX, newZ, kCollisionRadius);
    double heightDelta = sample.height - m_location[1];

    auto result = resolveTerrainCollision(
            heightDelta, kMaxStepUp, effectiveSpeed, kMass,
            terrainObstacleStrength(sample.type));

    switch (result.outcome) {
    case TerrainCollisionOutcome::Pass:
        break;
    case TerrainCollisionOutcome::SpeedBump:
        takeDamage(result.damage);
        step *= result.speedMultiplier;
        newX = m_location[0] + nx * step;
        newZ = m_location[2] + nz * step;
        break;
    case TerrainCollisionOutcome::CrashThrough:
        takeDamage(result.damage);
        step *= result.speedMultiplier;
        newX = m_location[0] + nx * step;
        newZ = m_location[2] + nz * step;
        break;
    case TerrainCollisionOutcome::HardStop:
        takeDamage(result.damage);
        m_hasTarget = false;
        return;
    }

    m_location[0] = newX;
    m_location[2] = newZ;
    m_yaw = std::atan2(nz, nx);
}

void Vehicle::applyGravity(double dt)
{
    double groundH = m_map.heightAt(m_location[0], m_location[2]);
    m_location[1] = m_body.applyGravity(dt, m_location[1], groundH, kGravity);
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
    : Vehicle(env, location, name, faction, EntityType::AirVehicle, map, speed)
    , m_altitude(altitude)
{
}

void AirVehicle::update(grid::libsim::DeltaType delta)
{
    Vehicle::update(delta);

    // Air vehicles fly at a fixed altitude above terrain
    m_location[1] = m_map.heightAt(m_location[0], m_location[2]) + m_altitude;
}

} // namespace battlegrid
