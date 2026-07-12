#ifndef GRID_PHYSICS_PHYSICS_WORLD_HPP_INCLUDED
#define GRID_PHYSICS_PHYSICS_WORLD_HPP_INCLUDED

#include "libphysics/collision_body.hpp"

#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace grid::physics {

/**
 * @brief Deterministic collision detection and impulse resolution.
 *
 * Owns a registry of CollisionBody objects and runs a fixed-timestep
 * simulation to detect overlaps, compute contact normals, and resolve
 * elastic impulses. Dynamic body transforms are authoritative in Box3D;
 * the game layer supplies velocity intentions, calls step(), and dispatches
 * the resulting Collision records as events.
 *
 * The fixed timestep guarantees frame-rate-independent, deterministic
 * results — critical for distributed simulation (DIS/HLA) where every
 * federate must agree on collision outcomes.
 *
 * Simulation mode
 * ───────────────
 * In **real-time mode** the accumulator is capped so the physics never
 * runs more than a small number of sub-steps per frame.  Fidelity may
 * degrade under heavy load (fewer sub-steps, coarser shapes).
 *
 * In **accuracy mode** the accumulator is NOT capped — every sub-step
 * is executed regardless of wall-clock cost.  The simulation clock
 * stretches but the physics remains fully accurate.
 */
class PhysicsWorld {
public:
    explicit PhysicsWorld(double fixedTimestep = 1.0 / 120.0);
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;
    PhysicsWorld(PhysicsWorld&&) noexcept;
    PhysicsWorld& operator=(PhysicsWorld&&) noexcept;

    // ── Body registry ───────────────────────────────────────────────

    void addBody(const CollisionBody& body);
    void removeBody(const std::string& name);
    void addStaticBox(const std::string& name, const Vec3& center, const Vec3& halfExtents);
    void removeStaticBox(const std::string& name);
    void clearStaticBoxes();

    /// Update an externally controlled body's position before step().
    void updateBodyPosition(const std::string& name,
                            double x, double y, double z);

    /// Read-only access to a registered body (nullptr if not found).
    const CollisionBody* body(const std::string& name) const;

    /// Read a body's transform from the Box3D simulation world.
    std::optional<Vec3> simulatedBodyPosition(const std::string& name) const;

    /// Set the Box3D velocity for a dynamic body.
    void setSimulatedBodyVelocity(const std::string& name, const Vec3& velocity);
    std::optional<Vec3> simulatedBodyVelocity(const std::string& name) const;
    /// Steer an upright body's yaw in radians without teleporting it.
    /// Returns false if the body does not exist or @p yaw is not finite.
    bool setSimulatedBodyYaw(const std::string& name, double yaw);
    /// Read an upright body's yaw in radians from the Box3D simulation.
    std::optional<double> simulatedBodyYaw(const std::string& name) const;
    /// Whether an upward static contact was seen recently enough to jump.
    /// This includes a brief grace period so input is not coupled to a
    /// particular fixed-step contact query.
    bool simulatedBodyGrounded(const std::string& name) const;
    /// Whether the body has a current Box3D contact with static geometry.
    bool simulatedBodyTouchesStatic(const std::string& name) const;

    /// Set world gravity. Bodies opt in with CollisionBody::gravityScale.
    void setGravity(const Vec3& gravity);

    std::size_t bodyCount() const { return m_bodies.size(); }
    std::size_t staticBoxCount() const;

    // ── Simulation ──────────────────────────────────────────────────

    /// Advance the physics simulation by @p dt seconds.
    /// Returns all collisions detected during the sub-steps.
    std::vector<Collision> step(double dt);

    /// Coefficient of restitution (0 = perfectly inelastic, 1 = elastic).
    void setRestitution(double e) { m_restitution = e; }
    double restitution() const { return m_restitution; }

    void setFixedTimestep(double ts) { m_fixedTimestep = ts; }
    double fixedTimestep() const { return m_fixedTimestep; }

    /// Maximum sub-steps per step() call in real-time mode.
    /// 0 = accuracy mode (unlimited sub-steps).
    void setMaxSubSteps(int n) { m_maxSubSteps = n; }
    int maxSubSteps() const { return m_maxSubSteps; }

private:
    struct Backend;

    /// Run one fixed-timestep sub-step.
    void subStep(double dt, std::vector<Collision>& out);

    /// Broad-phase: collect potentially overlapping pairs.
    void broadPhase(std::vector<std::pair<std::string, std::string>>& pairs) const;

    /// Narrow-phase: test a single pair and, if overlapping, compute
    /// the contact and append a Collision to @p out.
    void narrowPhase(const CollisionBody& a, const CollisionBody& b,
                     double dt, std::vector<Collision>& out) const;

    /// Compute elastic impulse magnitude given masses, relative velocity
    /// along the normal, and restitution.
    static double computeImpulse(double massA, double massB,
                                 double relVelAlongNormal,
                                 double restitution);

    std::unordered_map<std::string, CollisionBody> m_bodies;
    std::unique_ptr<Backend> m_backend;
    double m_fixedTimestep;
    double m_accumulator = 0.0;
    double m_frameDt     = 0.0;
    double m_simulationTime = 0.0;
    double m_restitution = 0.5;
    int    m_maxSubSteps = 8;  // 0 = accuracy mode
};

} // namespace grid::physics

#endif // GRID_PHYSICS_PHYSICS_WORLD_HPP_INCLUDED
