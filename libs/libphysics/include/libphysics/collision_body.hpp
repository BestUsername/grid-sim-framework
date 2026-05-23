#ifndef GRID_PHYSICS_COLLISION_BODY_HPP_INCLUDED
#define GRID_PHYSICS_COLLISION_BODY_HPP_INCLUDED

#include <array>
#include <string>

namespace grid::physics {

/// 3-component vector used throughout the physics library.
using Vec3 = std::array<double, 3>;

/**
 * @brief Physical properties of a collision-capable body.
 *
 * Registered with PhysicsWorld so that it can detect overlaps,
 * compute impulses, and report collisions.  The owning game code
 * is responsible for calling PhysicsWorld::updateBody() each frame
 * to keep position in sync with the simulation.
 */
struct CollisionBody {
    std::string name;      ///< Unique identifier (matches agent name).
    Vec3 position{};       ///< Centre position in world space.
    Vec3 prevPosition{};   ///< Position last frame (for velocity estimation).
    double mass   = 1.0;   ///< Mass in kg (0 = infinite / immovable).
    double radius = 0.5;   ///< Bounding-sphere radius (LOD 0).
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
