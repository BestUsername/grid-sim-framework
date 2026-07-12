#include "player_controller.hpp"
#include "battlegrid_world.hpp"

#include <algorithm>
#include <cmath>

namespace battlegrid {

PlayerController::PlayerController(Soldier& soldier, const TerrainMap& map,
                                   BattleGridWorld& world, InputMap& input)
    : m_soldier(soldier)
    , m_map(map)
    , m_world(world)
    , m_input(input)
{
}

void PlayerController::updateCamera(double dt)
{
    if (m_input.pressed(GameAction::ToggleCamera)) {
        m_cameraMode = (m_cameraMode == CameraMode::FirstPerson)
            ? CameraMode::ThirdPerson : CameraMode::FirstPerson;
    }

    // Delta inputs (mouse): instantaneous angular change
    m_yaw   += m_input.delta(GameAction::LookX) * m_mouseSensitivity;
    m_pitch -= m_input.delta(GameAction::LookY) * m_mouseSensitivity;

    // Continuous inputs (sticks, keys): angular velocity × dt
    m_yaw   += m_input.axis(GameAction::LookX) * m_stickLookSpeed * dt;
    m_pitch -= m_input.axis(GameAction::LookY) * m_stickLookSpeed * dt;

    constexpr double maxPitch = 1.4;
    m_pitch = std::clamp(m_pitch, -maxPitch, maxPitch);

    m_tpDistance -= m_input.delta(GameAction::Zoom);
    m_tpDistance = std::clamp(m_tpDistance, 3.0, 30.0);
}

void PlayerController::update(double dt, const PositionSnapshot& positions)
{
    // ── Edge-triggered actions ──────────────────────────────────────
    if (m_input.pressed(GameAction::Interact))
        tryMountDismount(positions);
    if (m_input.pressed(GameAction::Shout))
        shout();

    // ── Camera look / zoom / toggle ─────────────────────────────────
    updateCamera(dt);

    // ── Movement ────────────────────────────────────────────────────
    double moveX = m_input.axis(GameAction::MoveX);
    double moveZ = m_input.axis(GameAction::MoveZ);
    bool hasMove = std::abs(moveX) > 0.01 || std::abs(moveZ) > 0.01;

    double worldX = 0.0, worldZ = 0.0;

    if (hasMove) {
        // Normalize so diagonal movement isn't faster
        double len = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (len > 1.0) { moveX /= len; moveZ /= len; }

        // Rotate movement by camera yaw.
        double cosY = std::cos(m_yaw);
        double sinY = std::sin(m_yaw);
        worldX = -moveZ * cosY - moveX * sinY;
        worldZ = -moveZ * sinY + moveX * cosY;

        // Face the world-space movement direction (visible in third-person
        // because the camera orbit and model yaw are now decoupled).
        double facingYaw = std::atan2(worldZ, worldX);
        if (m_vehicle)
            m_vehicle->setYaw(facingYaw);
        else
            m_soldier.setYaw(facingYaw);
    }

    // Sprint: analog factor from 0 (walk) to 1 (full sprint)
    float sprintFactor = std::clamp(m_input.axis(GameAction::Sprint), 0.0f, 1.0f);
    double speedMul = 1.0 + static_cast<double>(sprintFactor);

    if (m_vehicle) {
        if (hasMove) {
            // Drive the vehicle
            const COORD& vpos = m_vehicle->location();
            double moveStep = m_vehicle->speed() * std::max(dt, 0.3);
            COORD target{
                vpos[0] + worldX * moveStep,
                0.0,
                vpos[2] + worldZ * moveStep
            };
            m_vehicle->setMoveTarget(target);
        } else {
            m_vehicle->clearMoveTarget();
        }
    } else {
        // ── On foot: horizontal movement + vertical physics ─────────
        const COORD& pos = m_soldier.location();
        double newX = pos[0];
        double newZ = pos[2];

        double currentGroundH = m_map.heightAt(pos[0], pos[2]);
        bool grounded = m_body.isGrounded(pos[1], currentGroundH);

        if (hasMove) {
            m_soldier.setSpeedMultiplier(speedMul);

            // When airborne, use full speed (no terrain penalty)
            double speedFactor = 1.0;
            if (grounded) {
                speedFactor = terrainSpeedFactor(m_map.at(
                    static_cast<size_t>(std::max(0.0, pos[0])),
                    static_cast<size_t>(std::max(0.0, pos[2]))));
            }

            double effectiveSpeed = m_soldier.speed() * speedMul * speedFactor;
            double step = effectiveSpeed * dt;

            double candidateX = newX + worldX * step;
            double candidateZ = newZ + worldZ * step;

            // Slope/wall check: sample the whole footprint, not just
            // the centre (works both grounded and airborne).
            double targetGroundH = m_map.maxHeightInRadius(
                    candidateX, candidateZ, kCollisionRadius);
            if (!grid::physics::KinematicBody::isTooSteep(
                       pos[1], targetGroundH, kMaxStepUp)) {
                newX = candidateX;
                newZ = candidateZ;
            }
        }

        // Jump (only when grounded)
        if (m_input.pressed(GameAction::Jump))
            m_body.tryJump(pos[1], currentGroundH, kJumpSpeed);

        // Gravity
        double newGroundH = m_map.heightAt(newX, newZ);
        double newY = m_body.applyGravity(dt, pos[1], newGroundH, kGravity);

        m_soldier.set_location(COORD{newX, newY, newZ});
    }
}

COORD PlayerController::desiredCameraPosition() const
{
    const COORD& pos = m_snapshotPos;

    if (m_cameraMode == CameraMode::FirstPerson) {
        double eyeH = m_vehicle ? 2.5 : kEyeHeight;
        return COORD{pos[0], pos[1] + eyeH, pos[2]};
    }

    // Third person: behind and above the soldier
    double camX = pos[0] - std::cos(m_yaw) * m_tpDistance;
    double camZ = pos[2] - std::sin(m_yaw) * m_tpDistance;
    double camY = pos[1] + m_tpHeight;
    return COORD{camX, camY, camZ};
}

COORD PlayerController::desiredCameraTarget() const
{
    const COORD& pos = m_snapshotPos;

    if (m_cameraMode == CameraMode::FirstPerson) {
        double eyeH = m_vehicle ? 2.5 : kEyeHeight;
        double lookDist = 10.0;
        return COORD{
            pos[0] + std::cos(m_yaw) * lookDist * std::cos(m_pitch),
            pos[1] + eyeH + std::sin(m_pitch) * lookDist,
            pos[2] + std::sin(m_yaw) * lookDist * std::cos(m_pitch)
        };
    }

    // Third person: look at soldier's upper body
    return COORD{pos[0], pos[1] + 1.0, pos[2]};
}

void PlayerController::tryMountDismount(const PositionSnapshot& positions)
{
    if (m_vehicle) {
        // Dismount — re-register the soldier's collision body.
        m_vehicle->dismount();
        grid::physics::CollisionBody body;
        body.name         = m_soldier.name();
        body.position     = {m_soldier.location()[0],
                             m_soldier.location()[1],
                             m_soldier.location()[2]};
        body.prevPosition = body.position;
        body.mass         = Soldier::kMass;
        body.radius       = Soldier::kCollisionRadius;
        body.motion       = grid::physics::BodyMotion::Dynamic;
        m_world.physicsWorld().addBody(body);
        m_vehicle = nullptr;
        m_soldier.setSpeedMultiplier(1.0);
    } else {
        // Try to mount the nearest vehicle within 4 units
        Vehicle* v = m_world.findNearestVehicle(m_snapshotPos, 4.0, positions);
        if (v && !v->hasDriver()) {
            if (v->mount(&m_soldier)) {
                m_vehicle = v;
                // Remove the soldier's collision body so it doesn't
                // collide with the vehicle it's riding.
                m_world.physicsWorld().removeBody(m_soldier.name());
            }
        }
    }
}

void PlayerController::shout()
{
    m_soldier.communicate(grid::libsim::Senses::Hearing, "Hey! Over here!");
}

} // namespace battlegrid
