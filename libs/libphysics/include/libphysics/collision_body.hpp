#ifndef GRID_PHYSICS_COLLISION_BODY_HPP_INCLUDED
#define GRID_PHYSICS_COLLISION_BODY_HPP_INCLUDED

#include <array>
#include <string>

namespace grid::physics {

/// 3-component vector used throughout the physics library.
using Vec3 = std::array<double, 3>;

enum class BodyMotion {
    Kinematic,
    Dynamic,
};

/// Geometric primitive used by Box3D for a CollisionBody.
enum class CollisionShape {
    Sphere,
    Capsule,
    Box,
};

/**
 * @brief Physical properties of a collision-capable body.
 *
 * Registered with PhysicsWorld so that it can detect overlaps,
 * compute impulses, and report collisions.  The owning game code
 * is responsible for setting its intended velocity. Dynamic bodies have
 * their transforms advanced and resolved by PhysicsWorld.
 */
struct CollisionBody {
    std::string name;      ///< Unique identifier (matches agent name).
    Vec3 position{};       ///< Centre position in world space.
    Vec3 prevPosition{};   ///< Position last frame (for velocity estimation).
    double mass   = 1.0;   ///< Mass in kg (0 = infinite / immovable).
    /// Sphere radius, or the radius of a capsule's hemispherical ends.
    double radius = 0.5;
    /// Shape primitive. Defaults to Sphere for backwards compatibility.
    CollisionShape shape = CollisionShape::Sphere;
    /// Capsule's total end-to-end height, including its hemispherical ends.
    double capsuleHeight = 1.0;
    /// Half dimensions of an axis-aligned box hull.
    Vec3 boxHalfExtents{0.5, 0.5, 0.5};
    BodyMotion motion = BodyMotion::Kinematic;
    double gravityScale = 0.0; ///< Multiplier for the world's gravity.
    bool lockVerticalMotion = false;
    /// Lock roll and pitch. Use lockYawRotation to also lock yaw.
    bool lockRotation = false;
    bool lockYawRotation = true;
};

/**
 * @brief Result of a collision between two bodies.
 *
 * Produced by PhysicsWorld::step() and consumed by the game layer
 * to create CollisionEvents for the participating entities.
 */
struct Collision {
    std::string nameA;
    std::string nameB;
    Vec3 normal{};         ///< Contact normal, pointing from A toward B.
    Vec3 relativeVelocity{}; ///< Velocity of A relative to B at impact.
    double impulse = 0.0;  ///< Scalar impulse magnitude (N·s).
    double massA   = 0.0;
    double massB   = 0.0;
    double penetration = 0.0; ///< Overlap depth along the normal.
};

} // namespace grid::physics

#endif // GRID_PHYSICS_COLLISION_BODY_HPP_INCLUDED
