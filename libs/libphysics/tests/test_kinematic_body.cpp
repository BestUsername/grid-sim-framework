#include "libphysics/kinematic_body.hpp"

#include <gtest/gtest.h>
#include <cmath>

using grid::physics::KinematicBody;

// ── isGrounded ──────────────────────────────────────────────────────

TEST(KinematicBodyTest, GroundedWhenOnSurface)
{
    KinematicBody body;
    EXPECT_TRUE(body.isGrounded(0.0, 0.0));
}

TEST(KinematicBodyTest, GroundedWithinEpsilon)
{
    KinematicBody body;
    EXPECT_TRUE(body.isGrounded(0.04, 0.0));
}

TEST(KinematicBodyTest, NotGroundedAboveEpsilon)
{
    KinematicBody body;
    EXPECT_FALSE(body.isGrounded(1.0, 0.0));
}

TEST(KinematicBodyTest, GroundedOnElevatedTerrain)
{
    KinematicBody body;
    EXPECT_TRUE(body.isGrounded(5.02, 5.0));
}

// ── tryJump ─────────────────────────────────────────────────────────

TEST(KinematicBodyTest, JumpWhenGrounded)
{
    KinematicBody body;
    EXPECT_TRUE(body.tryJump(0.0, 0.0, 8.0));
    EXPECT_DOUBLE_EQ(body.verticalVelocity, 8.0);
}

TEST(KinematicBodyTest, JumpFailsWhenAirborne)
{
    KinematicBody body;
    EXPECT_FALSE(body.tryJump(3.0, 0.0, 8.0));
    EXPECT_DOUBLE_EQ(body.verticalVelocity, 0.0);
}

// ── applyGravity ────────────────────────────────────────────────────

TEST(KinematicBodyTest, GravityAcceleratesDownward)
{
    KinematicBody body;
    double y = body.applyGravity(0.1, 10.0, 0.0, 20.0);
    // velocity after: 0 - 20*0.1 = -2.0
    // y: 10.0 + (-2.0)*0.1 = 9.8
    EXPECT_NEAR(y, 9.8, 1e-9);
    EXPECT_NEAR(body.verticalVelocity, -2.0, 1e-9);
}

TEST(KinematicBodyTest, LandsOnGround)
{
    KinematicBody body;
    body.verticalVelocity = -50.0; // falling fast
    double y = body.applyGravity(0.1, 0.5, 0.0, 20.0);
    EXPECT_DOUBLE_EQ(y, 0.0);
    EXPECT_DOUBLE_EQ(body.verticalVelocity, 0.0);
}

TEST(KinematicBodyTest, JumpThenFall)
{
    KinematicBody body;
    body.tryJump(0.0, 0.0, 8.0);

    double y = 0.0;
    double ground = 0.0;
    double dt = 0.01;
    double gravity = 20.0;

    // Rise
    for (int i = 0; i < 40; ++i) {
        y = body.applyGravity(dt, y, ground, gravity);
    }
    EXPECT_GT(y, 0.5); // should be well above ground

    // Eventually return to ground
    for (int i = 0; i < 200; ++i) {
        y = body.applyGravity(dt, y, ground, gravity);
    }
    EXPECT_DOUBLE_EQ(y, 0.0);
    EXPECT_DOUBLE_EQ(body.verticalVelocity, 0.0);
}

TEST(KinematicBodyTest, GravityLandsOnElevatedTerrain)
{
    KinematicBody body;
    body.verticalVelocity = -10.0;
    double y = body.applyGravity(0.1, 5.2, 5.0, 20.0);
    EXPECT_DOUBLE_EQ(y, 5.0);
    EXPECT_DOUBLE_EQ(body.verticalVelocity, 0.0);
}

// ── isTooSteep ──────────────────────────────────────────────────────

TEST(KinematicBodyTest, FlatTerrainNotSteep)
{
    EXPECT_FALSE(KinematicBody::isTooSteep(0.0, 0.0, 1.0));
}

TEST(KinematicBodyTest, SmallStepNotSteep)
{
    EXPECT_FALSE(KinematicBody::isTooSteep(0.0, 0.8, 1.0));
}

TEST(KinematicBodyTest, LargeCliffIsSteep)
{
    // Land (0) to Mountain (5) — definitely too steep for maxStep 1.0
    EXPECT_TRUE(KinematicBody::isTooSteep(0.0, 5.0, 1.0));
}

TEST(KinematicBodyTest, DownhillNeverSteep)
{
    // Stepping down is always allowed
    EXPECT_FALSE(KinematicBody::isTooSteep(5.0, 0.0, 1.0));
}

TEST(KinematicBodyTest, ExactMaxStepNotSteep)
{
    // Exactly at boundary — not too steep (uses >)
    EXPECT_FALSE(KinematicBody::isTooSteep(0.0, 1.0, 1.0));
}
