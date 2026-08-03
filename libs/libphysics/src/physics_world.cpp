#include "libphysics/physics_world.hpp"

#include <box3d/box3d.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace grid::physics {

namespace {

constexpr double kGroundNormalThreshold = 0.5;
constexpr double kGroundGracePeriod = 0.1;
constexpr double kYawAngularGain = 12.0;
constexpr double kMaxYawAngularSpeed = 10.0;

double boundingRadius(const CollisionBody& body)
{
    switch (body.shape) {
    case CollisionShape::Sphere:
        return body.radius;
    case CollisionShape::Capsule:
        return std::max(body.radius, body.capsuleHeight * 0.5);
    case CollisionShape::Box:
        return std::sqrt(
            body.boxHalfExtents[0] * body.boxHalfExtents[0]
            + body.boxHalfExtents[1] * body.boxHalfExtents[1]
            + body.boxHalfExtents[2] * body.boxHalfExtents[2]);
    case CollisionShape::Cylinder:
        return std::sqrt(body.radius * body.radius
                         + body.cylinderHeight * body.cylinderHeight * 0.25);
    }
    return body.radius;
}

} // namespace

struct PhysicsWorld::Backend {
    b3WorldId world;
    std::unordered_map<std::string, b3BodyId> bodies;
    std::unordered_map<std::string, b3ShapeId> shapes;
    std::unordered_map<std::string, b3JointId> wheelJoints;
    std::unordered_map<std::string, b3BodyId> staticBoxes;
    std::unordered_map<std::string, double> groundedUntil;

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
        definition.gravityScale = static_cast<float>(body.gravityScale);
        definition.motionLocks.linearY = body.lockVerticalMotion;
        definition.motionLocks.angularX = body.lockRotation;
        definition.motionLocks.angularY = body.lockRotation && body.lockYawRotation;
        definition.motionLocks.angularZ = body.lockRotation;
        definition.position = {
            static_cast<float>(body.position[0]),
            static_cast<float>(body.position[1]),
            static_cast<float>(body.position[2])};
        definition.name = body.name.c_str();

        b3BodyId bodyId = b3CreateBody(world, &definition);
        b3ShapeDef shapeDefinition = b3DefaultShapeDef();
        shapeDefinition.enableContactEvents = true;
        shapeDefinition.enableHitEvents = true;

        switch (body.shape) {
        case CollisionShape::Sphere: {
            b3Sphere sphere{};
            sphere.radius = static_cast<float>(body.radius);
            shapes[body.name] = b3CreateSphereShape(bodyId, &shapeDefinition, &sphere);
            break;
        }
        case CollisionShape::Capsule: {
            const double height = std::max(body.capsuleHeight, 2.0 * body.radius);
            const float halfSegment = static_cast<float>((height - 2.0 * body.radius) * 0.5);
            b3Capsule capsule{};
            capsule.center1 = {0.0f, -halfSegment, 0.0f};
            capsule.center2 = {0.0f, halfSegment, 0.0f};
            capsule.radius = static_cast<float>(body.radius);
            shapes[body.name] = b3CreateCapsuleShape(bodyId, &shapeDefinition, &capsule);
            break;
        }
        case CollisionShape::Box: {
            b3BoxHull box = b3MakeBoxHull(
                static_cast<float>(body.boxHalfExtents[0]),
                static_cast<float>(body.boxHalfExtents[1]),
                static_cast<float>(body.boxHalfExtents[2]));
            shapes[body.name] = b3CreateHullShape(bodyId, &shapeDefinition, &box.base);
            break;
        }
        case CollisionShape::Cylinder: {
            // Box3D cylinders are created around their local Y axis. Rotate
            // this tire hull so its axle is the body's local X axis.
            b3HullData* cylinder = b3CreateCylinder(
                static_cast<float>(body.cylinderHeight),
                static_cast<float>(body.radius), 0.0f, 12);
            b3Transform tireTransform{
                {0.0f, 0.0f, 0.0f},
                b3MakeQuatFromAxisAngle({0.0f, 0.0f, 1.0f},
                                        std::numbers::pi_v<float> * 0.5f)};
            shapeDefinition.baseMaterial.friction = 1.2f;
            shapes[body.name] = b3CreateTransformedHullShape(
                bodyId, &shapeDefinition, cylinder, tireTransform, {1.0f, 1.0f, 1.0f});
            b3DestroyHull(cylinder);
            break;
        }
        }
        if (body.motion == BodyMotion::Dynamic) {
            b3MassData massData = b3Body_GetMassData(bodyId);
            if (massData.mass > 0.0f) {
                massData.inertia = b3MulSM(
                    static_cast<float>(body.mass / massData.mass), massData.inertia);
            }
            massData.mass = static_cast<float>(body.mass);
            b3Body_SetMassData(bodyId, massData);
        }
        bodies.emplace(body.name, bodyId);
        groundedUntil.erase(body.name);
    }

    void removeBody(const std::string& name)
    {
        for (auto it = wheelJoints.begin(); it != wheelJoints.end();) {
            const b3JointId joint = it->second;
            const b3BodyId bodyA = b3Joint_GetBodyA(joint);
            const b3BodyId bodyB = b3Joint_GetBodyB(joint);
            const auto bodyIt = bodies.find(name);
            if (bodyIt != bodies.end()
                && (B3_ID_EQUALS(bodyA, bodyIt->second) || B3_ID_EQUALS(bodyB, bodyIt->second))) {
                b3DestroyJoint(joint, false);
                it = wheelJoints.erase(it);
            } else {
                ++it;
            }
        }
        const auto it = bodies.find(name);
        if (it == bodies.end()) {
            return;
        }

        b3DestroyBody(it->second);
        bodies.erase(it);
        shapes.erase(name);
        groundedUntil.erase(name);
    }

    bool addWheelJoint(const WheelJoint& wheel)
    {
        if (wheelJoints.contains(wheel.name)
            || !bodies.contains(wheel.chassisName) || !bodies.contains(wheel.wheelName)) {
            return false;
        }

        b3WheelJointDef definition = b3DefaultWheelJointDef();
        definition.base.bodyIdA = bodies.at(wheel.chassisName);
        definition.base.bodyIdB = bodies.at(wheel.wheelName);
        definition.base.localFrameA.p = {
            static_cast<float>(wheel.chassisAnchor[0]),
            static_cast<float>(wheel.chassisAnchor[1]),
            static_cast<float>(wheel.chassisAnchor[2])};
        // The joint's X axis is suspension travel (up); Z is the wheel axle.
        // This cyclic frame maps X→Y, Y→Z, and Z→X in chassis coordinates.
        const b3Quat frameRotation{{0.5f, 0.5f, 0.5f}, 0.5f};
        definition.base.localFrameA.q = frameRotation;
        definition.base.localFrameB.q = frameRotation;
        // Tires must not collide with their own chassis, but remain normal
        // dynamic colliders for terrain, actors, and other vehicles.
        definition.base.collideConnected = false;
        definition.enableSuspensionSpring = true;
        definition.suspensionHertz = static_cast<float>(wheel.suspensionHertz);
        definition.suspensionDampingRatio = static_cast<float>(wheel.suspensionDampingRatio);
        definition.enableSuspensionLimit = true;
        definition.lowerSuspensionLimit = static_cast<float>(-wheel.suspensionTravel);
        definition.upperSuspensionLimit = static_cast<float>(wheel.suspensionTravel);
        definition.enableSpinMotor = true;
        definition.maxSpinTorque = static_cast<float>(wheel.maxDriveTorque);
        definition.enableSteering = wheel.steering;
        definition.steeringHertz = 8.0f;
        definition.steeringDampingRatio = 0.85f;
        definition.maxSteeringTorque = static_cast<float>(wheel.maxSteeringTorque);
        definition.enableSteeringLimit = wheel.steering;
        definition.lowerSteeringLimit = static_cast<float>(-wheel.steeringLimit);
        definition.upperSteeringLimit = static_cast<float>(wheel.steeringLimit);
        wheelJoints.emplace(wheel.name, b3CreateWheelJoint(world, &definition));
        return true;
    }

    void removeWheelJoint(const std::string& name)
    {
        if (const auto it = wheelJoints.find(name); it != wheelJoints.end()) {
            b3DestroyJoint(it->second, false);
            wheelJoints.erase(it);
        }
    }

    void setWheelJointDrive(const std::string& name, double spinSpeed)
    {
        if (const auto it = wheelJoints.find(name); it != wheelJoints.end()) {
            b3WheelJoint_SetSpinMotorSpeed(it->second, static_cast<float>(spinSpeed));
        }
    }

    void setWheelJointSteering(const std::string& name, double steeringAngle)
    {
        if (const auto it = wheelJoints.find(name); it != wheelJoints.end()) {
            b3WheelJoint_SetTargetSteeringAngle(
                it->second, static_cast<float>(steeringAngle));
        }
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

    void clearStaticBoxes()
    {
        for (const auto& [name, bodyId] : staticBoxes) {
            b3DestroyBody(bodyId);
        }
        staticBoxes.clear();
    }

    void updatePosition(const std::string& name, const Vec3& position)
    {
        const auto it = bodies.find(name);
        if (it == bodies.end()) {
            return;
        }

        const b3Quat rotation = b3Body_GetRotation(it->second);
        b3Body_SetTransform(
            it->second,
            {static_cast<float>(position[0]), static_cast<float>(position[1]), static_cast<float>(position[2])},
            rotation);
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

    std::optional<Vec3> velocity(const std::string& name) const
    {
        const auto it = bodies.find(name);
        if (it == bodies.end()) {
            return std::nullopt;
        }

        const b3Vec3 velocity = b3Body_GetLinearVelocity(it->second);
        return Vec3{velocity.x, velocity.y, velocity.z};
    }

    bool setYaw(const std::string& name, double yaw)
    {
        const auto it = bodies.find(name);
        if (it == bodies.end() || !std::isfinite(yaw)) {
            return false;
        }

        const b3Quat rotation = b3Body_GetRotation(it->second);
        const double currentYaw = std::atan2(
            2.0 * (rotation.s * rotation.v.y + rotation.v.x * rotation.v.z),
            1.0 - 2.0 * (rotation.v.y * rotation.v.y + rotation.v.x * rotation.v.x));
        const double yawError = std::remainder(yaw - currentYaw, 2.0 * std::numbers::pi);
        const double angularVelocity = std::clamp(
            yawError * kYawAngularGain, -kMaxYawAngularSpeed, kMaxYawAngularSpeed);
        b3Body_SetAngularVelocity(it->second, {0.0f, static_cast<float>(angularVelocity), 0.0f});
        return true;
    }

    std::optional<double> yaw(const std::string& name) const
    {
        const auto it = bodies.find(name);
        if (it == bodies.end()) {
            return std::nullopt;
        }

        const b3Quat rotation = b3Body_GetRotation(it->second);
        return std::atan2(
            2.0 * (rotation.s * rotation.v.y + rotation.v.x * rotation.v.z),
            1.0 - 2.0 * (rotation.v.y * rotation.v.y + rotation.v.x * rotation.v.x));
    }

    bool contactsGround(const std::string& name) const
    {
        const auto bodyIt = bodies.find(name);
        const auto shapeIt = shapes.find(name);
        if (bodyIt == bodies.end() || shapeIt == shapes.end()) {
            return false;
        }

        const int capacity = b3Body_GetContactCapacity(bodyIt->second);
        if (capacity == 0) {
            return false;
        }
        std::vector<b3ContactData> contacts(static_cast<size_t>(capacity));
        const int count = b3Body_GetContactData(bodyIt->second, contacts.data(), capacity);
        for (int index = 0; index < count; ++index) {
            const auto& contact = contacts[index];
            const bool bodyIsA = B3_ID_EQUALS(contact.shapeIdA, shapeIt->second);
            for (int manifoldIndex = 0; manifoldIndex < contact.manifoldCount; ++manifoldIndex) {
                const auto& normal = contact.manifolds[manifoldIndex].normal;
                const double upwardNormal = bodyIsA ? -normal.y : normal.y;
                if (upwardNormal >= kGroundNormalThreshold) {
                    return true;
                }
            }
        }
        return false;
    }

    void updateGrounded(double simulationTime)
    {
        for (const auto& [name, bodyId] : bodies) {
            if (contactsGround(name)) {
                groundedUntil[name] = simulationTime + kGroundGracePeriod;
            }
        }
    }

    bool grounded(const std::string& name, double simulationTime) const
    {
        const auto it = groundedUntil.find(name);
        return it != groundedUntil.end() && it->second >= simulationTime;
    }

    bool touchesStatic(const std::string& name) const
    {
        const auto bodyIt = bodies.find(name);
        const auto shapeIt = shapes.find(name);
        if (bodyIt == bodies.end() || shapeIt == shapes.end()) {
            return false;
        }

        const int capacity = b3Body_GetContactCapacity(bodyIt->second);
        if (capacity == 0) {
            return false;
        }
        std::vector<b3ContactData> contacts(static_cast<size_t>(capacity));
        const int count = b3Body_GetContactData(bodyIt->second, contacts.data(), capacity);
        for (int index = 0; index < count; ++index) {
            const b3ShapeId otherShape = B3_ID_EQUALS(contacts[index].shapeIdA, shapeIt->second)
                ? contacts[index].shapeIdB
                : contacts[index].shapeIdA;
            const b3BodyId otherBody = b3Shape_GetBody(otherShape);
            for (const auto& [staticName, staticBody] : staticBoxes) {
                if (B3_ID_EQUALS(otherBody, staticBody)) {
                    return true;
                }
            }
        }
        return false;
    }

    std::optional<std::string> nameForShape(b3ShapeId shapeId) const
    {
        for (const auto& [name, candidate] : shapes) {
            if (B3_ID_EQUALS(candidate, shapeId)) {
                return name;
            }
        }
        return std::nullopt;
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
    for (auto it = m_wheelJoints.begin(); it != m_wheelJoints.end();) {
        if (it->second.chassisName == name || it->second.wheelName == name) {
            it = m_wheelJoints.erase(it);
        } else {
            ++it;
        }
    }
    m_bodies.erase(name);
    m_backend->removeBody(name);
}

bool PhysicsWorld::addWheelJoint(const WheelJoint& wheel)
{
    if (wheel.name.empty() || m_wheelJoints.contains(wheel.name)
        || !m_bodies.contains(wheel.chassisName) || !m_bodies.contains(wheel.wheelName)) {
        return false;
    }
    if (!m_backend->addWheelJoint(wheel)) {
        return false;
    }
    m_wheelJoints.emplace(wheel.name, wheel);
    return true;
}

void PhysicsWorld::removeWheelJoint(const std::string& name)
{
    m_backend->removeWheelJoint(name);
    m_wheelJoints.erase(name);
}

const WheelJoint* PhysicsWorld::wheelJoint(const std::string& name) const
{
    const auto it = m_wheelJoints.find(name);
    return it != m_wheelJoints.end() ? &it->second : nullptr;
}

void PhysicsWorld::setWheelJointDrive(const std::string& name, double spinSpeed)
{
    if (auto it = m_wheelJoints.find(name); it != m_wheelJoints.end()) {
        it->second.driveSpeed = spinSpeed;
        m_backend->setWheelJointDrive(name, spinSpeed);
    }
}

void PhysicsWorld::setWheelJointSteering(const std::string& name, double steeringAngle)
{
    if (auto it = m_wheelJoints.find(name); it != m_wheelJoints.end()) {
        it->second.targetSteeringAngle = steeringAngle;
        m_backend->setWheelJointSteering(name, steeringAngle);
    }
}

void PhysicsWorld::addStaticBox(const std::string& name, const Vec3& center, const Vec3& halfExtents)
{
    m_backend->addStaticBox(name, center, halfExtents);
}

void PhysicsWorld::removeStaticBox(const std::string& name)
{
    m_backend->removeStaticBox(name);
}

void PhysicsWorld::clearStaticBoxes()
{
    m_backend->clearStaticBoxes();
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

std::optional<Vec3> PhysicsWorld::simulatedBodyVelocity(const std::string& name) const
{
    return m_backend->velocity(name);
}

bool PhysicsWorld::setSimulatedBodyYaw(const std::string& name, double yaw)
{
    return m_backend->setYaw(name, yaw);
}

std::optional<double> PhysicsWorld::simulatedBodyYaw(const std::string& name) const
{
    return m_backend->yaw(name);
}

bool PhysicsWorld::simulatedBodyGrounded(const std::string& name) const
{
    return m_backend->grounded(name, m_simulationTime);
}

bool PhysicsWorld::simulatedBodyTouchesStatic(const std::string& name) const
{
    return m_backend->touchesStatic(name);
}

void PhysicsWorld::setGravity(const Vec3& gravity)
{
    b3World_SetGravity(
        m_backend->world,
        {static_cast<float>(gravity[0]), static_cast<float>(gravity[1]),
         static_cast<float>(gravity[2])});
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

    for (auto& [name, body] : m_bodies) {
        if (body.motion == BodyMotion::Dynamic) {
            if (const auto position = m_backend->position(name)) {
                body.prevPosition = body.position;
                body.position = *position;
            }
        }
    }

    return collisions;
}

void PhysicsWorld::subStep(double dt, std::vector<Collision>& out)
{
    b3World_Step(m_backend->world, static_cast<float>(dt), 4);
    m_simulationTime += dt;
    m_backend->updateGrounded(m_simulationTime);

    const b3ContactEvents events = b3World_GetContactEvents(m_backend->world);
    bool hasDynamicBodies = false;
    for (const auto& [name, body] : m_bodies) {
        hasDynamicBodies = hasDynamicBodies || body.motion == BodyMotion::Dynamic;
    }
    if (hasDynamicBodies) {
        for (int index = 0; index < events.hitCount; ++index) {
            const auto& hit = events.hitEvents[index];
            const auto nameA = m_backend->nameForShape(hit.shapeIdA);
            const auto nameB = m_backend->nameForShape(hit.shapeIdB);
            if (!nameA || !nameB) {
                continue;
            }

            const auto& bodyA = m_bodies.at(*nameA);
            const auto& bodyB = m_bodies.at(*nameB);
            const double invMassA = bodyA.mass > 0.0 ? 1.0 / bodyA.mass : 0.0;
            const double invMassB = bodyB.mass > 0.0 ? 1.0 / bodyB.mass : 0.0;
            const double denominator = invMassA + invMassB;
            const double impulse = denominator > 1e-12
                ? (1.0 + m_restitution) * hit.approachSpeed / denominator
                : 0.0;
            const b3Vec3 velocityA = b3Body_GetLinearVelocity(m_backend->bodies.at(*nameA));
            const b3Vec3 velocityB = b3Body_GetLinearVelocity(m_backend->bodies.at(*nameB));

            out.push_back({
                *nameA,
                *nameB,
                {hit.normal.x, hit.normal.y, hit.normal.z},
                {velocityA.x - velocityB.x, velocityA.y - velocityB.y, velocityA.z - velocityB.z},
                impulse,
                bodyA.mass,
                bodyB.mass,
                0.0});
        }
        return;
    }

    // Kinematic bodies are externally positioned, so retain deterministic
    // overlap records for their compatibility use case.
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
            double sumR = boundingRadius(*a) + boundingRadius(*b);
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
    // Kinematic compatibility path: approximate non-spherical shapes with
    // their enclosing spheres. Dynamic bodies use Box3D's exact shapes.
    double dx = b.position[0] - a.position[0];
    double dy = b.position[1] - a.position[1];
    double dz = b.position[2] - a.position[2];
    double distSq = dx * dx + dy * dy + dz * dz;
    double sumR = boundingRadius(a) + boundingRadius(b);

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
