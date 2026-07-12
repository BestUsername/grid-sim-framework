#include "libphysics/physics_world.hpp"
#include "libphysics/collision_body.hpp"
#include "libphysics/collision_event.hpp"

#include <gtest/gtest.h>
#include <cmath>
#include <algorithm>
#include <limits>
#include <string>

using grid::physics::PhysicsWorld;
using grid::physics::CollisionBody;
using grid::physics::CollisionEvent;
using grid::physics::Collision;
using grid::physics::Vec3;

// ── Body registry ───────────────────────────────────────────────────

TEST(PhysicsWorldTest, AddAndRemoveBody)
{
    PhysicsWorld pw;
    CollisionBody b;
    b.name = "A";
    b.position = {1.0, 0.0, 0.0};
    b.mass = 10.0;
    b.radius = 0.5;

    pw.addBody(b);
    EXPECT_EQ(pw.bodyCount(), 1u);
    EXPECT_NE(pw.body("A"), nullptr);
    EXPECT_EQ(pw.body("A")->shape, grid::physics::CollisionShape::Sphere);

    pw.removeBody("A");
    EXPECT_EQ(pw.bodyCount(), 0u);
    EXPECT_EQ(pw.body("A"), nullptr);
}

TEST(PhysicsWorldTest, AddAndRemoveStaticBox)
{
    PhysicsWorld pw;
    pw.addStaticBox("ground", {0.0, -0.5, 0.0}, {10.0, 0.5, 10.0});
    EXPECT_EQ(pw.staticBoxCount(), 1u);

    pw.addStaticBox("ground", {0.0, -1.0, 0.0}, {10.0, 1.0, 10.0});
    EXPECT_EQ(pw.staticBoxCount(), 1u);

    pw.removeStaticBox("ground");
    EXPECT_EQ(pw.staticBoxCount(), 0u);

    pw.addStaticBox("ground-a", {0.0, -0.5, 0.0}, {10.0, 0.5, 10.0});
    pw.addStaticBox("ground-b", {10.0, -0.5, 0.0}, {10.0, 0.5, 10.0});
    pw.clearStaticBoxes();
    EXPECT_EQ(pw.staticBoxCount(), 0u);
}

TEST(PhysicsWorldTest, UpdateBodyPosition)
{
    PhysicsWorld pw;
    CollisionBody b;
    b.name = "A";
    b.position = {0.0, 0.0, 0.0};
    pw.addBody(b);

    pw.updateBodyPosition("A", 5.0, 1.0, 3.0);
    const auto* body = pw.body("A");
    ASSERT_NE(body, nullptr);
    EXPECT_DOUBLE_EQ(body->position[0], 5.0);
    EXPECT_DOUBLE_EQ(body->position[1], 1.0);
    EXPECT_DOUBLE_EQ(body->position[2], 3.0);
}

TEST(PhysicsWorldTest, SimulatedPositionTracksRegisteredBody)
{
    PhysicsWorld pw;
    CollisionBody b;
    b.name = "A";
    b.position = {1.0, 2.0, 3.0};
    pw.addBody(b);

    auto initialPosition = pw.simulatedBodyPosition("A");
    ASSERT_TRUE(initialPosition.has_value());
    EXPECT_DOUBLE_EQ((*initialPosition)[0], 1.0);
    EXPECT_DOUBLE_EQ((*initialPosition)[1], 2.0);
    EXPECT_DOUBLE_EQ((*initialPosition)[2], 3.0);

    pw.updateBodyPosition("A", 5.0, 1.0, 3.0);
    auto updatedPosition = pw.simulatedBodyPosition("A");
    ASSERT_TRUE(updatedPosition.has_value());
    EXPECT_DOUBLE_EQ((*updatedPosition)[0], 5.0);
    EXPECT_DOUBLE_EQ((*updatedPosition)[1], 1.0);
    EXPECT_DOUBLE_EQ((*updatedPosition)[2], 3.0);
    EXPECT_FALSE(pw.simulatedBodyPosition("missing").has_value());
}

TEST(PhysicsWorldTest, DynamicBodyAdvancesFromSimulatedVelocity)
{
    PhysicsWorld pw(1.0 / 60.0);
    CollisionBody b;
    b.name = "dynamic";
    b.mass = 10.0;
    b.motion = grid::physics::BodyMotion::Dynamic;
    pw.addBody(b);

    pw.setSimulatedBodyVelocity("dynamic", {3.0, 0.0, 0.0});
    auto velocity = pw.simulatedBodyVelocity("dynamic");
    ASSERT_TRUE(velocity.has_value());
    EXPECT_DOUBLE_EQ((*velocity)[0], 3.0);
    pw.step(1.0 / 60.0);

    auto position = pw.simulatedBodyPosition("dynamic");
    ASSERT_TRUE(position.has_value());
    EXPECT_GT((*position)[0], 0.0);
    EXPECT_FALSE(pw.simulatedBodyVelocity("missing").has_value());
}

TEST(PhysicsWorldTest, SettingHullYawPreservesPositionAndVelocity)
{
    PhysicsWorld pw;
    CollisionBody hull;
    hull.name = "hull";
    hull.position = {3.25, 0.4, -2.5};
    hull.mass = 2000.0;
    hull.shape = grid::physics::CollisionShape::Box;
    hull.boxHalfExtents = {0.6, 0.4, 1.1};
    hull.motion = grid::physics::BodyMotion::Dynamic;
    hull.lockRotation = true;
    hull.lockYawRotation = false;
    pw.addBody(hull);
    pw.setSimulatedBodyVelocity(hull.name, {4.0, 0.0, -1.5});

    EXPECT_TRUE(pw.setSimulatedBodyYaw(hull.name, 1.5707963267948966));
    const auto position = pw.simulatedBodyPosition(hull.name);
    const auto velocity = pw.simulatedBodyVelocity(hull.name);
    ASSERT_TRUE(position.has_value());
    ASSERT_TRUE(velocity.has_value());
    EXPECT_NEAR((*position)[0], hull.position[0], 1e-6);
    EXPECT_NEAR((*position)[1], hull.position[1], 1e-6);
    EXPECT_NEAR((*position)[2], hull.position[2], 1e-6);
    EXPECT_DOUBLE_EQ((*velocity)[0], 4.0);
    EXPECT_DOUBLE_EQ((*velocity)[1], 0.0);
    EXPECT_DOUBLE_EQ((*velocity)[2], -1.5);
    EXPECT_FALSE(pw.setSimulatedBodyYaw("missing", 0.0));
    EXPECT_FALSE(pw.setSimulatedBodyYaw(hull.name, std::numeric_limits<double>::infinity()));
}

TEST(PhysicsWorldTest, DynamicBodyDoesNotPassThroughStaticBox)
{
    PhysicsWorld pw(1.0 / 60.0);
    pw.addStaticBox("wall", {2.0, 0.0, 0.0}, {0.5, 2.0, 2.0});

    CollisionBody b;
    b.name = "dynamic";
    b.mass = 10.0;
    b.radius = 0.5;
    b.motion = grid::physics::BodyMotion::Dynamic;
    pw.addBody(b);
    pw.setSimulatedBodyVelocity("dynamic", {10.0, 0.0, 0.0});

    for (int i = 0; i < 30; ++i) {
        pw.step(1.0 / 60.0);
    }

    auto position = pw.simulatedBodyPosition("dynamic");
    ASSERT_TRUE(position.has_value());
    EXPECT_LT((*position)[0], 1.1);
}

TEST(PhysicsWorldTest, UprightCapsuleRestsAtItsFootPosition)
{
    PhysicsWorld pw(1.0 / 60.0);
    pw.setMaxSubSteps(0);
    pw.setGravity({0.0, -10.0, 0.0});
    pw.addStaticBox("ground", {0.0, -0.5, 0.0}, {10.0, 0.5, 10.0});

    CollisionBody person;
    person.name = "person";
    person.position = {0.0, 3.0, 0.0};
    person.mass = 80.0;
    person.radius = 0.4;
    person.shape = grid::physics::CollisionShape::Capsule;
    person.capsuleHeight = 1.8;
    person.motion = grid::physics::BodyMotion::Dynamic;
    person.gravityScale = 1.0;
    person.lockRotation = true;
    pw.addBody(person);

    pw.step(1.0);

    const auto position = pw.simulatedBodyPosition("person");
    ASSERT_TRUE(position.has_value());
    EXPECT_NEAR((*position)[1], 0.9, 0.03);
}

TEST(PhysicsWorldTest, LowBoxHullPushesUprightCapsule)
{
    PhysicsWorld pw(1.0 / 60.0);
    pw.setMaxSubSteps(0);

    CollisionBody vehicle;
    vehicle.name = "vehicle";
    vehicle.position = {0.0, 0.4, 0.0};
    vehicle.mass = 2000.0;
    vehicle.shape = grid::physics::CollisionShape::Box;
    vehicle.boxHalfExtents = {0.6, 0.4, 1.1};
    vehicle.motion = grid::physics::BodyMotion::Dynamic;
    vehicle.lockVerticalMotion = true;
    vehicle.lockRotation = true;

    CollisionBody person;
    person.name = "person";
    person.position = {3.0, 0.9, 0.0};
    person.mass = 80.0;
    person.radius = 0.4;
    person.shape = grid::physics::CollisionShape::Capsule;
    person.capsuleHeight = 1.8;
    person.motion = grid::physics::BodyMotion::Dynamic;
    person.lockRotation = true;

    pw.addBody(vehicle);
    pw.addBody(person);
    pw.setSimulatedBodyVelocity("vehicle", {10.0, 0.0, 0.0});

    const auto collisions = pw.step(0.4);
    EXPECT_TRUE(std::any_of(collisions.begin(), collisions.end(), [](const Collision& collision) {
        return (collision.nameA == "vehicle" && collision.nameB == "person")
            || (collision.nameA == "person" && collision.nameB == "vehicle");
    }));

    const auto position = pw.simulatedBodyPosition("person");
    ASSERT_TRUE(position.has_value());
    EXPECT_GT((*position)[0], 3.0);
}

TEST(PhysicsWorldTest, DynamicBodiesReportBox3DCollisionData)
{
    PhysicsWorld pw(1.0 / 60.0);
    CollisionBody a;
    a.name = "A";
    a.position = {0.0, 0.0, 0.0};
    a.radius = 0.5;
    a.mass = 10.0;
    a.motion = grid::physics::BodyMotion::Dynamic;

    CollisionBody b = a;
    b.name = "B";
    b.position = {2.0, 0.0, 0.0};
    pw.addBody(a);
    pw.addBody(b);
    pw.setSimulatedBodyVelocity("A", {10.0, 0.0, 0.0});
    pw.setSimulatedBodyVelocity("B", {-10.0, 0.0, 0.0});

    const auto collisions = pw.step(0.2);
    ASSERT_FALSE(collisions.empty());
    EXPECT_GT(collisions.front().impulse, 0.0);
    EXPECT_GT(std::abs(collisions.front().normal[0]), 0.9);
    EXPECT_GT(std::abs(collisions.front().relativeVelocity[0]), 0.0);
}

TEST(PhysicsWorldTest, UpdatePreservesPreviousPosition)
{
    PhysicsWorld pw;
    CollisionBody b;
    b.name = "A";
    b.position = {1.0, 2.0, 3.0};
    pw.addBody(b);

    pw.updateBodyPosition("A", 4.0, 5.0, 6.0);
    const auto* body = pw.body("A");
    EXPECT_DOUBLE_EQ(body->prevPosition[0], 1.0);
    EXPECT_DOUBLE_EQ(body->prevPosition[1], 2.0);
    EXPECT_DOUBLE_EQ(body->prevPosition[2], 3.0);
}

// ── No collision ────────────────────────────────────────────────────

TEST(PhysicsWorldTest, NoBodiesNoCollisions)
{
    PhysicsWorld pw;
    auto results = pw.step(1.0 / 60.0);
    EXPECT_TRUE(results.empty());
}

TEST(PhysicsWorldTest, DistantBodiesNoCollision)
{
    PhysicsWorld pw;
    CollisionBody a;
    a.name = "A"; a.position = {0.0, 0.0, 0.0}; a.radius = 0.5; a.mass = 1.0;
    CollisionBody b;
    b.name = "B"; b.position = {10.0, 0.0, 0.0}; b.radius = 0.5; b.mass = 1.0;

    pw.addBody(a);
    pw.addBody(b);
    auto results = pw.step(1.0 / 60.0);
    EXPECT_TRUE(results.empty());
}

// ── Overlap detection ───────────────────────────────────────────────

TEST(PhysicsWorldTest, OverlappingBodiesCollide)
{
    PhysicsWorld pw(1.0 / 60.0);
    // Place two bodies overlapping and moving toward each other.
    CollisionBody a;
    a.name = "A"; a.position = {0.0, 0.0, 0.0}; a.prevPosition = {-1.0, 0.0, 0.0};
    a.radius = 0.5; a.mass = 1.0;
    CollisionBody b;
    b.name = "B"; b.position = {0.8, 0.0, 0.0}; b.prevPosition = {1.8, 0.0, 0.0};
    b.radius = 0.5; b.mass = 1.0;

    pw.addBody(a);
    pw.addBody(b);

    auto results = pw.step(1.0 / 60.0);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].nameA, "A");
    EXPECT_EQ(results[0].nameB, "B");
    EXPECT_GT(results[0].impulse, 0.0);
    EXPECT_GT(results[0].penetration, 0.0);
}

TEST(PhysicsWorldTest, SeparatingBodiesNoCollision)
{
    PhysicsWorld pw(1.0 / 60.0);
    // Overlapping but moving apart — should not collide.
    CollisionBody a;
    a.name = "A"; a.position = {0.0, 0.0, 0.0}; a.prevPosition = {0.5, 0.0, 0.0};
    a.radius = 0.5; a.mass = 1.0;
    CollisionBody b;
    b.name = "B"; b.position = {0.8, 0.0, 0.0}; b.prevPosition = {0.3, 0.0, 0.0};
    b.radius = 0.5; b.mass = 1.0;

    pw.addBody(a);
    pw.addBody(b);
    auto results = pw.step(1.0 / 60.0);
    EXPECT_TRUE(results.empty());
}

// ── Contact normal ──────────────────────────────────────────────────

TEST(PhysicsWorldTest, NormalPointsFromAToB)
{
    PhysicsWorld pw(1.0 / 60.0);
    CollisionBody a;
    a.name = "A"; a.position = {0.0, 0.0, 0.0}; a.prevPosition = {-1.0, 0.0, 0.0};
    a.radius = 0.5; a.mass = 1.0;
    CollisionBody b;
    b.name = "B"; b.position = {0.8, 0.0, 0.0}; b.prevPosition = {1.8, 0.0, 0.0};
    b.radius = 0.5; b.mass = 1.0;

    pw.addBody(a);
    pw.addBody(b);
    auto results = pw.step(1.0 / 60.0);
    ASSERT_EQ(results.size(), 1u);
    // Normal should point in +X (from A to B).
    EXPECT_GT(results[0].normal[0], 0.9);
    EXPECT_NEAR(results[0].normal[1], 0.0, 1e-9);
    EXPECT_NEAR(results[0].normal[2], 0.0, 1e-9);
}

// ── Impulse physics ─────────────────────────────────────────────────

TEST(PhysicsWorldTest, HeavyBodyTransfersMoreImpulse)
{
    // A heavy body hitting a light body should produce a larger impulse
    // than two equal-mass bodies at the same relative speed.
    PhysicsWorld pw1(1.0 / 60.0);
    pw1.setRestitution(1.0); // Perfectly elastic for clean comparison.

    // Equal masses
    CollisionBody a1;
    a1.name = "A"; a1.position = {0.0, 0.0, 0.0}; a1.prevPosition = {-1.0, 0.0, 0.0};
    a1.radius = 0.5; a1.mass = 1.0;
    CollisionBody b1;
    b1.name = "B"; b1.position = {0.8, 0.0, 0.0}; b1.prevPosition = {0.8, 0.0, 0.0};
    b1.radius = 0.5; b1.mass = 1.0;
    pw1.addBody(a1);
    pw1.addBody(b1);
    auto r1 = pw1.step(1.0 / 60.0);

    // Heavy A
    PhysicsWorld pw2(1.0 / 60.0);
    pw2.setRestitution(1.0);
    CollisionBody a2;
    a2.name = "A"; a2.position = {0.0, 0.0, 0.0}; a2.prevPosition = {-1.0, 0.0, 0.0};
    a2.radius = 0.5; a2.mass = 100.0;
    CollisionBody b2;
    b2.name = "B"; b2.position = {0.8, 0.0, 0.0}; b2.prevPosition = {0.8, 0.0, 0.0};
    b2.radius = 0.5; b2.mass = 1.0;
    pw2.addBody(a2);
    pw2.addBody(b2);
    auto r2 = pw2.step(1.0 / 60.0);

    ASSERT_FALSE(r1.empty());
    ASSERT_FALSE(r2.empty());
    EXPECT_GT(r2[0].impulse, r1[0].impulse);
}

TEST(PhysicsWorldTest, PerfectlyInelasticLowerImpulse)
{
    // Restitution 0 (inelastic) should produce less impulse than 1 (elastic)
    // for the same collision.
    auto makeWorld = [](double restitution) {
        PhysicsWorld pw(1.0 / 60.0);
        pw.setRestitution(restitution);
        CollisionBody a;
        a.name = "A"; a.position = {0.0, 0.0, 0.0}; a.prevPosition = {-1.0, 0.0, 0.0};
        a.radius = 0.5; a.mass = 1.0;
        CollisionBody b;
        b.name = "B"; b.position = {0.8, 0.0, 0.0}; b.prevPosition = {0.8, 0.0, 0.0};
        b.radius = 0.5; b.mass = 1.0;
        pw.addBody(a);
        pw.addBody(b);
        return pw;
    };

    auto pw0 = makeWorld(0.0);
    auto pw1 = makeWorld(1.0);
    auto r0 = pw0.step(1.0 / 60.0);
    auto r1 = pw1.step(1.0 / 60.0);

    ASSERT_FALSE(r0.empty());
    ASSERT_FALSE(r1.empty());
    EXPECT_LT(r0[0].impulse, r1[0].impulse);
}

// ── Fixed timestep accumulator ──────────────────────────────────────

TEST(PhysicsWorldTest, AccumulatorSubStepping)
{
    PhysicsWorld pw(0.01); // 100 Hz fixed step
    pw.setMaxSubSteps(0);  // Accuracy mode: unlimited.

    // No bodies, just verify step doesn't crash with a large dt.
    auto results = pw.step(0.05); // Should run 5 sub-steps.
    EXPECT_TRUE(results.empty());
}

TEST(PhysicsWorldTest, RealTimeModeDropsExcessTime)
{
    PhysicsWorld pw(0.01);
    pw.setMaxSubSteps(2);

    // Feed 0.05s — would need 5 sub-steps but cap is 2.
    // No bodies, so just verifying no crash and accumulator is drained.
    auto results = pw.step(0.05);
    EXPECT_TRUE(results.empty());

    // A second step with a normal dt should work fine.
    results = pw.step(0.01);
    EXPECT_TRUE(results.empty());
}

// ── CollisionEvent ──────────────────────────────────────────────────

TEST(CollisionEventTest, CarriesCorrectData)
{
    CollisionEvent evt("SoldierA", "Humvee_1",
                       {1.0, 0.0, 0.0}, {-5.0, 0.0, 0.0},
                       120.0, 80.0, 2000.0, 0.1);

    EXPECT_EQ(evt.GetKey(), "physics.collision");
    EXPECT_EQ(evt.thisEntity(), "SoldierA");
    EXPECT_EQ(evt.otherEntity(), "Humvee_1");
    EXPECT_DOUBLE_EQ(evt.impulse(), 120.0);
    EXPECT_DOUBLE_EQ(evt.thisMass(), 80.0);
    EXPECT_DOUBLE_EQ(evt.otherMass(), 2000.0);
    EXPECT_DOUBLE_EQ(evt.penetration(), 0.1);
    EXPECT_DOUBLE_EQ(evt.normal()[0], 1.0);
    EXPECT_DOUBLE_EQ(evt.relativeVelocity()[0], -5.0);
}

TEST(CollisionEventTest, CloneProducesIndependentCopy)
{
    CollisionEvent evt("A", "B", {0.0, 1.0, 0.0}, {0.0, -3.0, 0.0},
                       50.0, 10.0, 20.0, 0.05);
    auto cloned = evt.clone();
    auto* ce = dynamic_cast<CollisionEvent*>(cloned.get());
    ASSERT_NE(ce, nullptr);
    EXPECT_EQ(ce->thisEntity(), "A");
    EXPECT_EQ(ce->otherEntity(), "B");
    EXPECT_DOUBLE_EQ(ce->impulse(), 50.0);
}

// ── Penetration depth ───────────────────────────────────────────────

TEST(PhysicsWorldTest, PenetrationDepthIsCorrect)
{
    PhysicsWorld pw(1.0 / 60.0);
    CollisionBody a;
    a.name = "A"; a.position = {0.0, 0.0, 0.0}; a.prevPosition = {-1.0, 0.0, 0.0};
    a.radius = 0.5; a.mass = 1.0;
    CollisionBody b;
    // Distance = 0.6, sumR = 1.0, penetration = 0.4
    b.name = "B"; b.position = {0.6, 0.0, 0.0}; b.prevPosition = {1.6, 0.0, 0.0};
    b.radius = 0.5; b.mass = 1.0;

    pw.addBody(a);
    pw.addBody(b);
    auto results = pw.step(1.0 / 60.0);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_NEAR(results[0].penetration, 0.4, 1e-9);
}

// ── Deterministic pair ordering ─────────────────────────────────────

TEST(PhysicsWorldTest, DeterministicPairOrdering)
{
    // Add bodies in different orders, verify collision results are identical.
    auto run = [](const std::string& firstName, const std::string& secondName) {
        PhysicsWorld pw(1.0 / 60.0);
        CollisionBody a;
        a.name = firstName; a.position = {0.0, 0.0, 0.0}; a.prevPosition = {-1.0, 0.0, 0.0};
        a.radius = 0.5; a.mass = 1.0;
        CollisionBody b;
        b.name = secondName; b.position = {0.8, 0.0, 0.0}; b.prevPosition = {1.8, 0.0, 0.0};
        b.radius = 0.5; b.mass = 1.0;
        pw.addBody(a);
        pw.addBody(b);
        return pw.step(1.0 / 60.0);
    };

    auto r1 = run("Alpha", "Beta");
    auto r2 = run("Beta", "Alpha");

    ASSERT_EQ(r1.size(), 1u);
    ASSERT_EQ(r2.size(), 1u);
    // Name A should always be the lexicographically smaller name.
    EXPECT_EQ(r1[0].nameA, "Alpha");
    EXPECT_EQ(r2[0].nameA, "Alpha");
    EXPECT_DOUBLE_EQ(r1[0].impulse, r2[0].impulse);
}
