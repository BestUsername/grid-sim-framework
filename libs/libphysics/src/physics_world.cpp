#include "libphysics/physics_world.hpp"

#include <box3d/box3d.h>

#include <algorithm>
#include <cmath>

namespace grid::physics {

struct PhysicsWorld::Backend {
    b3WorldId world;
    std::unordered_map<std::string, b3BodyId> bodies;
    std::unordered_map<std::string, b3BodyId> staticBoxes;

    Backend()
    {
        b3WorldDef definition = b3DefaultWorldDef();
        definition.gravity = {0.0f, 0.0f, 0.0f};
        world = b3CreateWorld(&definition);
    }

    ~Backend()
    {
        b3DestroyWorld(world);
    }

    void addBody(const CollisionBody& body)
    {
        if (const auto existing = bodies.find(body.name); existing != bodies.end()) {
            b3DestroyBody(existing->second);
            bodies.erase(existing);
        }

        b3BodyDef definition = b3DefaultBodyDef();
        definition.type = body.motion == BodyMotion::Dynamic ? b3_dynamicBody : b3_kinematicBody;
        definition.gravityScale = 0.0f;
        definition.position = {
            static_cast<float>(body.position[0]),
            static_cast<float>(body.position[1]),
            static_cast<float>(body.position[2])};
        definition.name = body.name.c_str();

        b3BodyId bodyId = b3CreateBody(world, &definition);
        b3ShapeDef shapeDefinition = b3DefaultShapeDef();
        shapeDefinition.enableContactEvents = true;
        shapeDefinition.enableHitEvents = true;

        b3Sphere sphere{};
        sphere.radius = static_cast<float>(body.radius);
        b3CreateSphereShape(bodyId, &shapeDefinition, &sphere);
        if (body.motion == BodyMotion::Dynamic) {
            b3MassData massData = b3Body_GetMassData(bodyId);
            massData.mass = static_cast<float>(body.mass);
            b3Body_SetMassData(bodyId, massData);
        }
        bodies.emplace(body.name, bodyId);
    }

    void removeBody(const std::string& name)
    {
        const auto it = bodies.find(name);
        if (it == bodies.end()) {
            return;
        }

        b3DestroyBody(it->second);
        bodies.erase(it);
    }

    void addStaticBox(const std::string& name, const Vec3& center, const Vec3& halfExtents)
    {
        if (const auto existing = staticBoxes.find(name); existing != staticBoxes.end()) {
            b3DestroyBody(existing->second);
            staticBoxes.erase(existing);
        }

        b3BodyDef definition = b3DefaultBodyDef();
        definition.position = {
            static_cast<float>(center[0]),
            static_cast<float>(center[1]),
            static_cast<float>(center[2])};
        definition.name = name.c_str();

        b3BodyId bodyId = b3CreateBody(world, &definition);
        b3ShapeDef shapeDefinition = b3DefaultShapeDef();
        b3BoxHull box = b3MakeBoxHull(
            static_cast<float>(halfExtents[0]),
            static_cast<float>(halfExtents[1]),
            static_cast<float>(halfExtents[2]));
        b3CreateHullShape(bodyId, &shapeDefinition, &box.base);
        staticBoxes.emplace(name, bodyId);
    }

    void removeStaticBox(const std::string& name)
    {
        const auto it = staticBoxes.find(name);
        if (it == staticBoxes.end()) {
            return;
        }

        b3DestroyBody(it->second);
        staticBoxes.erase(it);
    }

    void updatePosition(const std::string& name, const Vec3& position)
    {
        const auto it = bodies.find(name);
        if (it == bodies.end()) {
            return;
        }

        b3BodyDef definition = b3DefaultBodyDef();
        b3Body_SetTransform(
            it->second,
            {static_cast<float>(position[0]), static_cast<float>(position[1]), static_cast<float>(position[2])},
            definition.rotation);
    }

    std::optional<Vec3> position(const std::string& name) const
    {
        const auto it = bodies.find(name);
        if (it == bodies.end()) {
            return std::nullopt;
        }

        const b3Pos position = b3Body_GetPosition(it->second);
        return Vec3{position.x, position.y, position.z};
    }

    void setVelocity(const std::string& name, const Vec3& velocity)
    {
        const auto it = bodies.find(name);
        if (it != bodies.end()) {
            b3Body_SetLinearVelocity(
                it->second,
                {static_cast<float>(velocity[0]), static_cast<float>(velocity[1]), static_cast<float>(velocity[2])});
        }
    }
};

PhysicsWorld::PhysicsWorld(double fixedTimestep)
    : m_fixedTimestep(fixedTimestep)
    , m_backend(std::make_unique<Backend>())
{
}

PhysicsWorld::~PhysicsWorld() = default;
PhysicsWorld::PhysicsWorld(PhysicsWorld&&) noexcept = default;
PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&&) noexcept = default;

// ── Body registry ───────────────────────────────────────────────────

void PhysicsWorld::addBody(const CollisionBody& body)
{
    m_bodies[body.name] = body;
    m_backend->addBody(body);
}

void PhysicsWorld::removeBody(const std::string& name)
{
    m_bodies.erase(name);
    m_backend->removeBody(name);
}

void PhysicsWorld::addStaticBox(const std::string& name, const Vec3& center, const Vec3& halfExtents)
{
    m_backend->addStaticBox(name, center, halfExtents);
}

void PhysicsWorld::removeStaticBox(const std::string& name)
{
    m_backend->removeStaticBox(name);
}

void PhysicsWorld::updateBodyPosition(const std::string& name,
                                      double x, double y, double z)
{
    auto it = m_bodies.find(name);
    if (it == m_bodies.end()) return;
    it->second.prevPosition = it->second.position;
    it->second.position = {x, y, z};
    m_backend->updatePosition(name, it->second.position);
}

const CollisionBody* PhysicsWorld::body(const std::string& name) const
{
    auto it = m_bodies.find(name);
    return it != m_bodies.end() ? &it->second : nullptr;
}

std::size_t PhysicsWorld::staticBoxCount() const
{
    return m_backend->staticBoxes.size();
}

std::optional<Vec3> PhysicsWorld::simulatedBodyPosition(const std::string& name) const
{
    return m_backend->position(name);
}

void PhysicsWorld::setSimulatedBodyVelocity(const std::string& name, const Vec3& velocity)
{
    m_backend->setVelocity(name, velocity);
}

// ── Simulation ──────────────────────────────────────────────────────

std::vector<Collision> PhysicsWorld::step(double dt)
{
    std::vector<Collision> collisions;

    m_frameDt = dt;
    m_accumulator += dt;

    int steps = 0;
    while (m_accumulator >= m_fixedTimestep) {
        if (m_maxSubSteps > 0 && steps >= m_maxSubSteps) {
            // Real-time mode: drop remaining time to keep up.
            m_accumulator = 0.0;
            break;
        }
        subStep(m_fixedTimestep, collisions);
        m_accumulator -= m_fixedTimestep;
        ++steps;
    }

    return collisions;
}

void PhysicsWorld::subStep(double dt, std::vector<Collision>& out)
{
    b3World_Step(m_backend->world, static_cast<float>(dt), 4);

    // Agents still own transforms during the compatibility phase. Keep the
    // legacy collision records until Box3D becomes transform-authoritative.
    // Broad phase: gather candidate pairs.
    std::vector<std::pair<std::string, std::string>> pairs;
    broadPhase(pairs);

    // Narrow phase: test each candidate pair.
    for (const auto& [nameA, nameB] : pairs) {
        const auto& a = m_bodies.at(nameA);
        const auto& b = m_bodies.at(nameB);
        narrowPhase(a, b, dt, out);
    }
}

void PhysicsWorld::broadPhase(
    std::vector<std::pair<std::string, std::string>>& pairs) const
{
    // O(n²) sweep — sufficient for the current entity count.
    // Replace with a spatial hash or BVH when entity count grows.
    std::vector<const CollisionBody*> bodies;
    bodies.reserve(m_bodies.size());
    for (const auto& [name, body] : m_bodies) {
        bodies.push_back(&body);
    }
    // Sort by name for deterministic pair ordering.
    std::sort(bodies.begin(), bodies.end(),
              [](const CollisionBody* a, const CollisionBody* b) {
                  return a->name < b->name;
              });

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            const auto* a = bodies[i];
            const auto* b = bodies[j];
            double dx = a->position[0] - b->position[0];
            double dy = a->position[1] - b->position[1];
            double dz = a->position[2] - b->position[2];
            double distSq = dx * dx + dy * dy + dz * dz;
            double sumR = a->radius + b->radius;
            if (distSq < sumR * sumR) {
                pairs.emplace_back(a->name, b->name);
            }
        }
    }
}

void PhysicsWorld::narrowPhase(const CollisionBody& a, const CollisionBody& b,
                               double dt,
                               std::vector<Collision>& out) const
{
    // Sphere-sphere narrow phase (matches broad phase for now).
    double dx = b.position[0] - a.position[0];
    double dy = b.position[1] - a.position[1];
    double dz = b.position[2] - a.position[2];
    double distSq = dx * dx + dy * dy + dz * dz;
    double sumR = a.radius + b.radius;

    if (distSq >= sumR * sumR) return;

    double dist = std::sqrt(distSq);
    double penetration = sumR - dist;

    // Contact normal (A → B).  Fall back to +X if bodies overlap exactly.
    Vec3 normal;
    if (dist > 1e-8) {
        normal = {dx / dist, dy / dist, dz / dist};
    } else {
        normal = {1.0, 0.0, 0.0};
    }

    // Estimate velocities from position deltas.
    // Use the frame dt (not the sub-step dt) because position deltas
    // span a full frame — dividing by a smaller sub-step would inflate
    // the estimated velocities.
    double velDt = (m_frameDt > 1e-12) ? m_frameDt : dt;
    double invDt = 1.0 / velDt;
    Vec3 velA = {
        (a.position[0] - a.prevPosition[0]) * invDt,
        (a.position[1] - a.prevPosition[1]) * invDt,
        (a.position[2] - a.prevPosition[2]) * invDt
    };
    Vec3 velB = {
        (b.position[0] - b.prevPosition[0]) * invDt,
        (b.position[1] - b.prevPosition[1]) * invDt,
        (b.position[2] - b.prevPosition[2]) * invDt
    };

    // Relative velocity of B w.r.t. A (standard convention with A→B normal).
    Vec3 relVel = {velB[0] - velA[0], velB[1] - velA[1], velB[2] - velA[2]};

    // Relative velocity along the contact normal.
    double relVelN = relVel[0] * normal[0]
                   + relVel[1] * normal[1]
                   + relVel[2] * normal[2];

    // Only resolve if bodies are approaching (relVelN < 0).
    if (relVelN >= 0.0) return;

    double impulse = computeImpulse(a.mass, b.mass, relVelN, m_restitution);

    Collision c;
    c.nameA = a.name;
    c.nameB = b.name;
    c.normal = normal;
    c.relativeVelocity = relVel;
    c.impulse = impulse;
    c.massA = a.mass;
    c.massB = b.mass;
    c.penetration = penetration;
    out.push_back(std::move(c));
}

double PhysicsWorld::computeImpulse(double massA, double massB,
                                    double relVelAlongNormal,
                                    double restitution)
{
    // Inverse masses (0 mass = infinite / immovable).
    double invA = (massA > 0.0) ? 1.0 / massA : 0.0;
    double invB = (massB > 0.0) ? 1.0 / massB : 0.0;

    double denom = invA + invB;
    if (denom < 1e-12) return 0.0; // Both immovable.

    return -(1.0 + restitution) * relVelAlongNormal / denom;
}

} // namespace grid::physics
