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
 * elastic impulses.  The game layer syncs entity positions each frame,
 * calls step(), and dispatches the resulting Collision records as events.
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

    /// Update a body's position.  Call once per frame before step().
    void updateBodyPosition(const std::string& name,
                            double x, double y, double z);

    /// Read-only access to a registered body (nullptr if not found).
    const CollisionBody* body(const std::string& name) const;

    /// Read a body's transform from the Box3D simulation world.
    std::optional<Vec3> simulatedBodyPosition(const std::string& name) const;

    std::size_t bodyCount() const { return m_bodies.size(); }

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
    double m_restitution = 0.5;
    int    m_maxSubSteps = 8;  // 0 = accuracy mode
};

} // namespace grid::physics

#endif // GRID_PHYSICS_PHYSICS_WORLD_HPP_INCLUDED
