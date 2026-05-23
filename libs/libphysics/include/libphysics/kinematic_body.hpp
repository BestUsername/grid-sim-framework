#ifndef GRID_PHYSICS_KINEMATIC_BODY_HPP_INCLUDED
#define GRID_PHYSICS_KINEMATIC_BODY_HPP_INCLUDED

namespace grid::physics {

/**
 * @brief Vertical physics state for a body affected by gravity.
 *
 * Tracks vertical velocity and provides helpers for gravity integration,
 * jump initiation, grounded checks, and slope limiting.  Designed to be
 * embedded in any entity that moves under gravity — infantry, vehicles, or
 * (eventually) aircraft with full flight-dynamics models.
 */
struct KinematicBody {
    double verticalVelocity = 0.0;

    /// True when the body rests on (or very close to) the ground surface.
    bool isGrounded(double posY, double groundY,
                    double epsilon = 0.05) const;

    /// Attempt a jump.  Sets verticalVelocity and returns true only when
    /// the body is currently grounded.
    bool tryJump(double posY, double groundY, double jumpSpeed,
                 double epsilon = 0.05);

    /// Integrate gravity for @p dt seconds.  If the resulting position
    /// falls through the ground, the body is landed (Y clamped,
    /// verticalVelocity zeroed).  Returns the new Y position.
    double applyGravity(double dt, double currentY, double groundY,
                        double gravity);

    /// True if stepping from the current foot height to a target ground
    /// height would require climbing a wall / slope that exceeds @p maxStepUp.
    static bool isTooSteep(double currentFootY, double targetGroundY,
                           double maxStepUp);
};

} // namespace grid::physics

#endif // GRID_PHYSICS_KINEMATIC_BODY_HPP_INCLUDED
