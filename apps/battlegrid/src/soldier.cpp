#include "soldier.hpp"

#include "libphysics/collision_event.hpp"
#include "libsim/sense_event.hpp"

#include <cmath>

namespace battlegrid {

Soldier::Soldier(I_ENVIRONMENT& env,
                 const COORD& location,
                 const std::string& name,
                 Faction faction,
                 const TerrainMap& map,
                 double speed,
                 float audioRange)
    : BASE_AGENT(env, location, name)
    , m_faction(faction)
    , m_map(map)
    , m_speed(speed)
    , m_audioRange(audioRange)
{
}

void Soldier::start()
{
    communicate(grid::libsim::Senses::Hearing,
                factionName(m_faction) + " soldier " + m_name + " reporting for duty!");
}

void Soldier::update(grid::libsim::DeltaType delta)
{
    BASE_AGENT::update(delta);
    double dt = delta.count();

    if (isDead()) {
        applyGravity(dt);
        return;
    }

    if (!m_playerControlled && !m_remoteOwned) {
        if (m_hasTarget) {
            moveTowardTarget(dt);
        }
        applyGravity(dt);
    }
}

void Soldier::takeDamage(double amount)
{
    if (isDead()) return;
    m_health -= amount;
    if (m_health <= 0.0) {
        m_health = 0.0;
        m_hasTarget = false;
        m_environment.getGameLog().log(
            m_name, "", grid::libsim::Senses::Sight,
            m_name + " has been killed!");
    }
}

void Soldier::on_event(grid::libevent::Event* event)
{
    if (isDead()) return;

    if (auto* col = dynamic_cast<grid::physics::CollisionEvent*>(event)) {
        m_environment.getGameLog().log(
            m_name, "", grid::libsim::Senses::Touch,
            "Collided with " + col->otherEntity() + "!");
        // Apply collision damage: force = impulse / mass
        double force = col->impulse() / col->thisMass();
        if (force > kDamageThreshold) {
            takeDamage(force - kDamageThreshold);
        }
        return;
    }

    // React to audio events (e.g., nearby gunfire, commands)
    auto* sense = dynamic_cast<grid::libsim::SenseEvent<NUMBER_TYPE, NUM_DIMENSIONS>*>(event);
    if (sense && sense->GetKey() == "sense.audio") {
        if (sense->perceivedIntensity() > 0.1f) {
            // Acknowledge hearing something
            m_environment.getGameLog().log(
                m_name, "", grid::libsim::Senses::Hearing,
                "Heard: " + sense->message());
        }
    }
}

void Soldier::setMoveTarget(const COORD& target)
{
    m_target    = target;
    m_hasTarget = true;
}

void Soldier::clearMoveTarget()
{
    m_hasTarget = false;
}

void Soldier::moveTowardTarget(double dt)
{
    COORD diff = m_target - m_location;
    // Only consider XZ movement direction
    double dx = diff[0];
    double dz = diff[2];
    double dist = std::sqrt(dx * dx + dz * dz);

    if (dist < 0.1) {
        m_hasTarget = false;
        return;
    }

    // Terrain speed modifier (only when grounded)
    auto ix = static_cast<size_t>(std::max(0.0, m_location[0]));
    auto iz = static_cast<size_t>(std::max(0.0, m_location[2]));
    TerrainType terrain = m_map.at(ix, iz);

    double speedFactor = isGrounded() ? terrainSpeedFactor(terrain) : 1.0;

    if (isGrounded() && !isTraversableByLand(terrain)) {
        m_hasTarget = false;
        return;
    }

    double effectiveSpeed = m_speed * m_speedMultiplier * speedFactor;
    double step = effectiveSpeed * dt;
    if (step > dist) step = dist;

    double nx = dx / dist;
    double nz = dz / dist;

    double newX = m_location[0] + nx * step;
    double newZ = m_location[2] + nz * step;

    // Slope/wall check: sample the whole footprint, not just the centre
    double targetGroundH = m_map.maxHeightInRadius(newX, newZ, kCollisionRadius);
    if (grid::physics::KinematicBody::isTooSteep(m_location[1], targetGroundH,
                                                  kMaxStepUp)) {
        m_hasTarget = false;
        return;
    }

    m_location[0] = newX;
    m_location[2] = newZ;

    // Update facing
    m_yaw = std::atan2(nz, nx);
}

void Soldier::jump()
{
    double groundH = m_map.heightAt(m_location[0], m_location[2]);
    m_body.tryJump(m_location[1], groundH, kJumpSpeed);
}

bool Soldier::isGrounded() const
{
    double terrainH = m_map.heightAt(m_location[0], m_location[2]);
    return m_body.isGrounded(m_location[1], terrainH);
}

void Soldier::applyGravity(double dt)
{
    double groundH = m_map.heightAt(m_location[0], m_location[2]);
    m_location[1] = m_body.applyGravity(dt, m_location[1], groundH, kGravity);
}

} // namespace battlegrid
