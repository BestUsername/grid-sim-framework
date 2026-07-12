#include "soldier.hpp"

#include "libphysics/collision_event.hpp"
#include "libsim/sense_event.hpp"

#include <cmath>
#include <utility>

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
        setMovementVelocity(0.0, 0.0);
        return;
    }

    if (!m_playerControlled && !m_remoteOwned) {
        if (m_hasTarget) {
            moveTowardTarget(dt);
        }
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
    setMovementVelocity(0.0, 0.0);
}

void Soldier::moveTowardTarget(double dt)
{
    COORD diff = m_target - m_location;
    // Only consider XZ movement direction
    double dx = diff[0];
    double dz = diff[2];
    double dist = std::sqrt(dx * dx + dz * dz);

    if (dist < 0.1) {
        clearMoveTarget();
        return;
    }

    double nx = dx / dist;
    double nz = dz / dist;
    double effectiveSpeed = std::min(m_speed * m_speedMultiplier, dist / std::max(dt, 1e-12));
    setMovementVelocity(nx * effectiveSpeed, nz * effectiveSpeed);

    // Update facing
    m_yaw = std::atan2(nz, nx);
}

void Soldier::jump()
{
    m_jumpRequested = true;
}

bool Soldier::consumeJumpRequest()
{
    return std::exchange(m_jumpRequested, false);
}

} // namespace battlegrid
