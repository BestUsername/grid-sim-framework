#include "civilian.hpp"

#include "libphysics/collision_event.hpp"
#include "libsim/sense_event.hpp"

#include <cmath>
#include <cstdlib>

namespace battlegrid {

Civilian::Civilian(I_ENVIRONMENT& env,
                   const COORD& location,
                   const std::string& name,
                   const TerrainMap& map,
                   double speed)
    : BASE_AGENT(env, location, name)
    , m_map(map)
    , m_speed(speed)
{
}

void Civilian::start()
{
    communicate(grid::libsim::Senses::Sight, m_name + " goes about their day.");
}

void Civilian::update(grid::libsim::DeltaType delta)
{
    BASE_AGENT::update(delta);
    if (m_remoteOwned) return;
    double dt = delta.count();

    if (isDead()) {
        applyGravity(dt);
        return;
    }

    if (m_fleeing && m_hasTarget) {
        moveTowardTarget(dt);
        if (!m_hasTarget) m_fleeing = false;
        applyGravity(dt);
        return;
    }

    m_wanderTimer += dt;
    if (m_wanderTimer >= m_wanderInterval || !m_hasTarget) {
        pickNewWanderTarget();
        m_wanderTimer = 0.0;
    }

    if (m_hasTarget) {
        moveTowardTarget(dt);
    }
    applyGravity(dt);
}

void Civilian::takeDamage(double amount)
{
    if (isDead()) return;
    m_health -= amount;
    if (m_health <= 0.0) {
        m_health = 0.0;
        m_hasTarget = false;
        m_fleeing = false;
        m_environment.getGameLog().log(
            m_name, "", grid::libsim::Senses::Sight,
            m_name + " has been killed!");
    }
}

void Civilian::on_event(grid::libevent::Event* event)
{
    if (isDead()) return;

    if (auto* col = dynamic_cast<grid::physics::CollisionEvent*>(event)) {
        m_environment.getGameLog().log(
            m_name, "", grid::libsim::Senses::Touch,
            "Collided with " + col->otherEntity() + "!");
        double force = col->impulse() / col->thisMass();
        if (force > kDamageThreshold) {
            takeDamage(force - kDamageThreshold);
        }
        return;
    }

    auto* sense = dynamic_cast<grid::libsim::SenseEvent<NUMBER_TYPE, NUM_DIMENSIONS>*>(event);
    if (!sense) return;

    // Flee from loud sounds (combat, explosions)
    if (sense->GetKey() == "sense.audio" && sense->perceivedIntensity() > 0.5f) {
        m_fleeing = true;
        // Run away from the sound source
        COORD away = m_location - sense->origin();
        double dx = away[0];
        double dz = away[2];
        double dist = std::sqrt(dx * dx + dz * dz);
        if (dist > 0.01) {
            double fleeDistance = 15.0;
            m_target = COORD{
                m_location[0] + (dx / dist) * fleeDistance,
                0.0,
                m_location[2] + (dz / dist) * fleeDistance
            };
            m_hasTarget = true;
        }
    }
}

void Civilian::pickNewWanderTarget()
{
    // Pick a random nearby land position
    double range = 10.0;
    for (int attempt = 0; attempt < 10; ++attempt) {
        double ox = (static_cast<double>(std::rand()) / RAND_MAX - 0.5) * 2.0 * range;
        double oz = (static_cast<double>(std::rand()) / RAND_MAX - 0.5) * 2.0 * range;
        double tx = m_location[0] + ox;
        double tz = m_location[2] + oz;

        auto ix = static_cast<size_t>(std::max(0.0, tx));
        auto iz = static_cast<size_t>(std::max(0.0, tz));
        if (m_map.inBounds(ix, iz) && isTraversableByLand(m_map.at(ix, iz))) {
            m_target = COORD{tx, 0.0, tz};
            m_hasTarget = true;
            return;
        }
    }
    m_hasTarget = false;
}

void Civilian::moveTowardTarget(double dt)
{
    COORD diff = m_target - m_location;
    double dx = diff[0];
    double dz = diff[2];
    double dist = std::sqrt(dx * dx + dz * dz);

    if (dist < 0.5) {
        m_hasTarget = false;
        return;
    }

    auto ix = static_cast<size_t>(std::max(0.0, m_location[0]));
    auto iz = static_cast<size_t>(std::max(0.0, m_location[2]));
    TerrainType terrain = m_map.at(ix, iz);

    double speedFactor = isGrounded() ? terrainSpeedFactor(terrain) : 1.0;

    if (isGrounded() && !isTraversableByLand(terrain)) {
        m_hasTarget = false;
        return;
    }

    double effectiveSpeed = m_speed * speedFactor;
    if (m_fleeing) effectiveSpeed *= 1.5; // panic sprint
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
    m_yaw = std::atan2(nz, nx);
}

void Civilian::jump()
{
    double groundH = m_map.heightAt(m_location[0], m_location[2]);
    m_body.tryJump(m_location[1], groundH, kJumpSpeed);
}

bool Civilian::isGrounded() const
{
    double terrainH = m_map.heightAt(m_location[0], m_location[2]);
    return m_body.isGrounded(m_location[1], terrainH);
}

void Civilian::applyGravity(double dt)
{
    double groundH = m_map.heightAt(m_location[0], m_location[2]);
    m_location[1] = m_body.applyGravity(dt, m_location[1], groundH, kGravity);
}

} // namespace battlegrid
