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

    if (auto* landVehicle = dynamic_cast<LandVehicle*>(m_vehicle)) {
        // Land vehicles use car controls rather than camera-relative walking:
        // W drives forward, S reverses, and A/D steer the front wheels.
        landVehicle->setDrivingControls(
            -moveZ,
            moveX);
        return;
    }

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
        if (m_vehicle) {
            m_vehicle->setYaw(facingYaw);
        } else {
            m_soldier.setYaw(facingYaw);
        }
    }

    // Sprint: analog factor from 0 (walk) to 1 (full sprint)
    float sprintFactor = std::clamp(m_input.axis(GameAction::Sprint), 0.0f, 1.0f);
    double speedMul = 1.0 + static_cast<double>(sprintFactor);

    if (m_vehicle) {
        if (hasMove) {
            m_vehicle->setMoveTarget(COORD{
                m_vehicle->location()[0] + worldX * m_vehicle->speed(),
                m_vehicle->location()[1],
                m_vehicle->location()[2] + worldZ * m_vehicle->speed()});
        } else {
            m_vehicle->clearMoveTarget();
        }
    } else {
        if (hasMove) {
            m_soldier.setSpeedMultiplier(speedMul);
            m_soldier.setMovementVelocity(
                worldX * m_soldier.speed() * speedMul,
                worldZ * m_soldier.speed() * speedMul);
        } else {
            m_soldier.setMovementVelocity(0.0, 0.0);
        }

        // Box3D verifies whether the jump can take effect during the
        // authoritative physics step.
        if (m_input.pressed(GameAction::Jump))
            m_soldier.jump();
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
        m_world.dismountSoldier(m_soldier, *m_vehicle);
        m_vehicle = nullptr;
        m_soldier.setSpeedMultiplier(1.0);
    } else {
        // Try to mount the nearest vehicle within 4 units
        Vehicle* v = m_world.findNearestVehicle(m_snapshotPos, 4.0, positions);
        if (v && !v->hasDriver()) {
            if (m_world.mountSoldier(m_soldier, *v)) {
                m_vehicle = v;
            }
        }
    }
}

void PlayerController::shout()
{
    m_soldier.communicate(grid::libsim::Senses::Hearing, "Hey! Over here!");
}

} // namespace battlegrid
