#ifndef BATTLEGRID_PLAYER_CONTROLLER_HPP_INCLUDED
#define BATTLEGRID_PLAYER_CONTROLLER_HPP_INCLUDED

#include "defines.hpp"
#include "input_map.hpp"
#include "soldier.hpp"
#include "terrain.hpp"
#include "vehicle.hpp"

#include "libphysics/kinematic_body.hpp"

#include <cmath>

namespace battlegrid {

class BattleGridWorld; // forward

enum class CameraMode {
    FirstPerson,
    ThirdPerson,
};

/**
 * @brief Handles player input and controls a Soldier agent.
 *
 * Reads action values from an InputMap each frame instead of
 * processing raw events directly.  Supports FPS and third-person
 * camera modes.
 */
class PlayerController {
public:
    explicit PlayerController(Soldier& soldier, const TerrainMap& map,
                              BattleGridWorld& world, InputMap& input);

    /// Apply accumulated input to the controlled soldier each frame.
    /// Must be called under engine.withAgentsLock() to serialise
    /// position writes with the engine thread.
    void update(double dt, const PositionSnapshot& positions);

    /// Update only camera angles, zoom, and mode toggle from input.
    /// Useful when camera motion should advance without applying movement.
    void updateCamera(double dt);

    /// Set the entity position from the current frame's snapshot.
    /// Must be called each frame before cameraPosition()/cameraTarget().
    void setSnapshotPosition(const COORD& pos) { m_snapshotPos = pos; }

    CameraMode cameraMode() const { return m_cameraMode; }

    /// Camera position in world space.
    COORD cameraPosition() const { return desiredCameraPosition(); }

    /// Camera look-at target in world space.
    COORD cameraTarget() const { return desiredCameraTarget(); }

    double yaw()   const { return m_yaw; }
    double pitch() const { return m_pitch; }

    /// True when the player is driving a vehicle.
    bool inVehicle() const { return m_vehicle != nullptr; }
    Vehicle* currentVehicle() const { return m_vehicle; }

private:
    void tryMountDismount(const PositionSnapshot& positions);
    void shout();
    COORD desiredCameraPosition() const;
    COORD desiredCameraTarget() const;

    Soldier&          m_soldier;
    const TerrainMap& m_map;
    BattleGridWorld&  m_world;
    InputMap&         m_input;
    Vehicle*          m_vehicle = nullptr;
    CameraMode        m_cameraMode = CameraMode::ThirdPerson;

    // Camera angles (radians)
    double m_yaw   = 0.0;
    double m_pitch = 0.3;

    // Third-person camera offset
    double m_tpDistance = 8.0;
    double m_tpHeight  = 4.0;

    // Sensitivity for continuous-rate look inputs (gamepad sticks, keys)
    double m_stickLookSpeed = 3.0;  // radians per second at full deflection

    // Sensitivity for delta-based look inputs (mouse)
    double m_mouseSensitivity = 0.003;

    // Position from the per-frame snapshot (thread-safe for camera)
    COORD m_snapshotPos{0.0, 0.0, 0.0};

    // Vertical physics
    grid::physics::KinematicBody m_body;
    static constexpr double kGravity     = 20.0; // m/s²
    static constexpr double kJumpSpeed   =  8.0; // m/s
    static constexpr double kMaxStepUp   =  1.2; // m

    // Collision footprint radius (matching Soldier)
    static constexpr double kCollisionRadius = 0.4;

    // Height offset for first person (eye level)
    static constexpr double kEyeHeight = 1.7;
};

} // namespace battlegrid

#endif // BATTLEGRID_PLAYER_CONTROLLER_HPP_INCLUDED
