#include "libphysics/kinematic_body.hpp"

namespace grid::physics {

bool KinematicBody::isGrounded(double posY, double groundY,
                               double epsilon) const
{
    return (posY - groundY) < epsilon;
}

bool KinematicBody::tryJump(double posY, double groundY,
                            double jumpSpeed, double epsilon)
{
    if (!isGrounded(posY, groundY, epsilon))
        return false;
    verticalVelocity = jumpSpeed;
    return true;
}

double KinematicBody::applyGravity(double dt, double currentY,
                                   double groundY, double gravity)
{
    verticalVelocity -= gravity * dt;
    double newY = currentY + verticalVelocity * dt;

    if (newY <= groundY) {
        newY = groundY;
        verticalVelocity = 0.0;
    }
    return newY;
}

bool KinematicBody::isTooSteep(double currentFootY, double targetGroundY,
                               double maxStepUp)
{
    return (targetGroundY - currentFootY) > maxStepUp;
}

} // namespace grid::physics
