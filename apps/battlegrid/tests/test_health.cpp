#include "soldier.hpp"
#include "civilian.hpp"
#include "vehicle.hpp"
#include "terrain.hpp"
#include "libphysics/collision_event.hpp"
#include "libsim/base_engine.hpp"

#include <gtest/gtest.h>

using namespace battlegrid;

namespace {

// Minimal engine that satisfies the I_ENVIRONMENT interface.
class TestEngine : public BASE_ENGINE {
public:
    TestEngine() : BASE_ENGINE(60) {}
};

// Small flat terrain for entity construction.
TerrainMap makeFlatMap()
{
    return TerrainMap(10, 10, TerrainType::Land);
}

// Helper: build a CollisionEvent delivering a given impulse to the receiver.
grid::physics::CollisionEvent makeHit(const std::string& self,
                                      const std::string& other,
                                      double impulse,
                                      double selfMass,
                                      double otherMass)
{
    grid::physics::Vec3 normal{1.0, 0.0, 0.0};
    grid::physics::Vec3 relVel{5.0, 0.0, 0.0};
    return grid::physics::CollisionEvent(
        self, other, normal, relVel,
        impulse, selfMass, otherMass, /*penetration=*/0.01);
}

} // namespace

// ── Soldier health ──────────────────────────────────────────────────

TEST(SoldierHealthTest, StartsAtFullHealth)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    EXPECT_DOUBLE_EQ(s.health(), Soldier::kMaxHealth);
    EXPECT_FALSE(s.isDead());
}

TEST(SoldierHealthTest, TakeDamageReducesHealth)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    s.takeDamage(30.0);
    EXPECT_DOUBLE_EQ(s.health(), 70.0);
    EXPECT_FALSE(s.isDead());
}

TEST(SoldierHealthTest, DiesAtZeroHealth)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    s.takeDamage(100.0);
    EXPECT_DOUBLE_EQ(s.health(), 0.0);
    EXPECT_TRUE(s.isDead());
}

TEST(SoldierHealthTest, OverkillClampsToZero)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    s.takeDamage(999.0);
    EXPECT_DOUBLE_EQ(s.health(), 0.0);
    EXPECT_TRUE(s.isDead());
}

TEST(SoldierHealthTest, NoDamageAfterDeath)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    s.takeDamage(100.0);
    EXPECT_TRUE(s.isDead());
    s.takeDamage(50.0);  // should be a no-op
    EXPECT_DOUBLE_EQ(s.health(), 0.0);
}

TEST(SoldierHealthTest, CollisionBelowThresholdNoDamage)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    // impulse / mass = 640 / 80 = 8.0, below threshold 10.0 (walking bump)
    auto evt = makeHit("S1", "Other", 640.0, Soldier::kMass, 70.0);
    s.on_event(&evt);
    EXPECT_DOUBLE_EQ(s.health(), Soldier::kMaxHealth);
}

TEST(SoldierHealthTest, CollisionAboveThresholdCausesDamage)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    // impulse / mass = 1200 / 80 = 15.0, above threshold 10.0 → damage = 5.0
    auto evt = makeHit("S1", "Other", 1200.0, Soldier::kMass, 2000.0);
    s.on_event(&evt);
    EXPECT_DOUBLE_EQ(s.health(), Soldier::kMaxHealth - 5.0);
}

// ── Civilian health ─────────────────────────────────────────────────

TEST(CivilianHealthTest, StartsAtFullHealth)
{
    TestEngine env;
    auto map = makeFlatMap();
    Civilian c(env, COORD{5.0, 0.0, 5.0}, "C1", map, 2.0);
    EXPECT_DOUBLE_EQ(c.health(), Civilian::kMaxHealth);
    EXPECT_FALSE(c.isDead());
}

TEST(CivilianHealthTest, DiesWhenHealthDepleted)
{
    TestEngine env;
    auto map = makeFlatMap();
    Civilian c(env, COORD{5.0, 0.0, 5.0}, "C1", map, 2.0);
    c.takeDamage(50.0);
    EXPECT_DOUBLE_EQ(c.health(), 0.0);
    EXPECT_TRUE(c.isDead());
}

TEST(CivilianHealthTest, CollisionDamage)
{
    TestEngine env;
    auto map = makeFlatMap();
    Civilian c(env, COORD{5.0, 0.0, 5.0}, "C1", map, 2.0);
    // impulse / mass = 1400 / 70 = 20.0 → damage = 10.0 (hit by vehicle)
    auto evt = makeHit("C1", "Truck", 1400.0, Civilian::kMass, 2000.0);
    c.on_event(&evt);
    EXPECT_DOUBLE_EQ(c.health(), Civilian::kMaxHealth - 10.0);
}

// ── Vehicle health ──────────────────────────────────────────────────

TEST(VehicleHealthTest, StartsAtFullHealth)
{
    TestEngine env;
    auto map = makeFlatMap();
    LandVehicle v(env, COORD{5.0, 0.0, 5.0}, "V1", Faction::Blue, map, 8.0);
    EXPECT_DOUBLE_EQ(v.health(), Vehicle::kMaxHealth);
    EXPECT_FALSE(v.isDead());
}

TEST(VehicleHealthTest, HighThreshold)
{
    TestEngine env;
    auto map = makeFlatMap();
    LandVehicle v(env, COORD{5.0, 0.0, 5.0}, "V1", Faction::Blue, map, 8.0);
    // impulse / mass = 60000 / 2000 = 30.0, below 50.0 threshold → no damage
    auto evt = makeHit("V1", "S1", 60000.0, Vehicle::kMass, 80.0);
    v.on_event(&evt);
    EXPECT_DOUBLE_EQ(v.health(), Vehicle::kMaxHealth);
}

TEST(VehicleHealthTest, DamageAboveThreshold)
{
    TestEngine env;
    auto map = makeFlatMap();
    LandVehicle v(env, COORD{5.0, 0.0, 5.0}, "V1", Faction::Blue, map, 8.0);
    // impulse / mass = 200000 / 2000 = 100.0 → damage = 50.0
    auto evt = makeHit("V1", "V2", 200000.0, Vehicle::kMass, 2000.0);
    v.on_event(&evt);
    EXPECT_DOUBLE_EQ(v.health(), Vehicle::kMaxHealth - 50.0);
}

TEST(VehicleHealthTest, Destruction)
{
    TestEngine env;
    auto map = makeFlatMap();
    LandVehicle v(env, COORD{5.0, 0.0, 5.0}, "V1", Faction::Blue, map, 8.0);
    v.takeDamage(500.0);
    EXPECT_TRUE(v.isDead());
    EXPECT_DOUBLE_EQ(v.health(), 0.0);
}

// ── Dead entities stop updating ─────────────────────────────────────

TEST(SoldierHealthTest, DeadSoldierIgnoresEvents)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    s.takeDamage(100.0);
    EXPECT_TRUE(s.isDead());
    size_t logsBefore = env.getGameLog().size();
    auto evt = makeHit("S1", "Truck", 10000.0, Soldier::kMass, 2000.0);
    s.on_event(&evt);
    // No new log entries — dead entities don't react
    EXPECT_EQ(env.getGameLog().size(), logsBefore);
}

TEST(CivilianHealthTest, DeadCivilianIgnoresEvents)
{
    TestEngine env;
    auto map = makeFlatMap();
    Civilian c(env, COORD{5.0, 0.0, 5.0}, "C1", map, 2.0);
    c.takeDamage(50.0);
    EXPECT_TRUE(c.isDead());
    size_t logsBefore = env.getGameLog().size();
    auto evt = makeHit("C1", "Truck", 10000.0, Civilian::kMass, 2000.0);
    c.on_event(&evt);
    EXPECT_EQ(env.getGameLog().size(), logsBefore);
}

TEST(SoldierHealthTest, DeadSoldierDoesNotMove)
{
    TestEngine env;
    auto map = makeFlatMap();
    Soldier s(env, COORD{5.0, 0.0, 5.0}, "S1", Faction::Blue, map, 3.0, 10.0f);
    s.setMoveTarget(COORD{20.0, 0.0, 20.0});
    s.takeDamage(100.0);
    EXPECT_TRUE(s.isDead());
    auto before = s.location();
    s.update(std::chrono::duration<double>(1.0));
    EXPECT_EQ(s.location(), before);
}

TEST(CivilianHealthTest, DeadCivilianDoesNotMove)
{
    TestEngine env;
    auto map = makeFlatMap();
    Civilian c(env, COORD{5.0, 0.0, 5.0}, "C1", map, 2.0);
    c.takeDamage(50.0);
    EXPECT_TRUE(c.isDead());
    auto before = c.location();
    c.update(std::chrono::duration<double>(1.0));
    EXPECT_EQ(c.location(), before);
}

// ── Terrain collision outcomes ──────────────────────────────────────

TEST(TerrainCollisionTest, PassWhenWithinStepUp)
{
    // heightDelta = 1.5, maxStepUp = 2.0 → Pass
    auto r = battlegrid::resolveTerrainCollision(1.5, 2.0, 10.0, 2000.0, 50000.0);
    EXPECT_EQ(r.outcome, battlegrid::TerrainCollisionOutcome::Pass);
    EXPECT_DOUBLE_EQ(r.damage, 0.0);
    EXPECT_DOUBLE_EQ(r.speedMultiplier, 1.0);
}

TEST(TerrainCollisionTest, PassWhenDownhill)
{
    // Negative heightDelta → Pass
    auto r = battlegrid::resolveTerrainCollision(-3.0, 2.0, 10.0, 2000.0, 50000.0);
    EXPECT_EQ(r.outcome, battlegrid::TerrainCollisionOutcome::Pass);
}

TEST(TerrainCollisionTest, SpeedBumpSmallExcess)
{
    // heightDelta = 2.8, maxStepUp = 2.0 → excess = 0.8 ≤ 1.0 → SpeedBump
    auto r = battlegrid::resolveTerrainCollision(2.8, 2.0, 10.0, 2000.0, 50000.0);
    EXPECT_EQ(r.outcome, battlegrid::TerrainCollisionOutcome::SpeedBump);
    EXPECT_GT(r.damage, 0.0);
    EXPECT_DOUBLE_EQ(r.speedMultiplier, 0.7);
    // damage = speed * excess * 0.5 = 10 * 0.8 * 0.5 = 4.0
    EXPECT_DOUBLE_EQ(r.damage, 4.0);
}

TEST(TerrainCollisionTest, HardStopAgainstMountain)
{
    // Land→Mountain: delta = 5.0, excess = 3.0, speed = 10, mass = 2000
    // momentum = 20000 < mountainStrength = 50000 → HardStop
    auto r = battlegrid::resolveTerrainCollision(5.0, 2.0, 10.0, 2000.0, 50000.0);
    EXPECT_EQ(r.outcome, battlegrid::TerrainCollisionOutcome::HardStop);
    EXPECT_DOUBLE_EQ(r.damage, 20.0);  // speed * 2
    EXPECT_DOUBLE_EQ(r.speedMultiplier, 0.0);
}

TEST(TerrainCollisionTest, CrashThroughAtHighSpeed)
{
    // Same mountain but speed = 30 → momentum = 60000 > 50000 → CrashThrough
    auto r = battlegrid::resolveTerrainCollision(5.0, 2.0, 30.0, 2000.0, 50000.0);
    EXPECT_EQ(r.outcome, battlegrid::TerrainCollisionOutcome::CrashThrough);
    EXPECT_DOUBLE_EQ(r.damage, 25.0);  // obstacleStrength / mass
    EXPECT_DOUBLE_EQ(r.speedMultiplier, 0.3);
}

TEST(TerrainCollisionTest, CrashThroughDirtMound)
{
    // Dirt/land obstacle strength = 5000, speed = 10, mass = 2000
    // momentum = 20000 > 5000 → CrashThrough even at moderate speed
    auto r = battlegrid::resolveTerrainCollision(4.0, 2.0, 10.0, 2000.0, 5000.0);
    EXPECT_EQ(r.outcome, battlegrid::TerrainCollisionOutcome::CrashThrough);
    EXPECT_DOUBLE_EQ(r.damage, 2.5);  // 5000 / 2000
    EXPECT_DOUBLE_EQ(r.speedMultiplier, 0.3);
}

TEST(TerrainCollisionTest, HardStopZeroStrength)
{
    // Zero obstacle strength and large excess → HardStop (not crash-through)
    auto r = battlegrid::resolveTerrainCollision(5.0, 2.0, 10.0, 2000.0, 0.0);
    EXPECT_EQ(r.outcome, battlegrid::TerrainCollisionOutcome::HardStop);
}

TEST(TerrainCollisionTest, ObstacleStrengthValues)
{
    EXPECT_DOUBLE_EQ(battlegrid::terrainObstacleStrength(battlegrid::TerrainType::Water), 0.0);
    EXPECT_DOUBLE_EQ(battlegrid::terrainObstacleStrength(battlegrid::TerrainType::Land), 5000.0);
    EXPECT_DOUBLE_EQ(battlegrid::terrainObstacleStrength(battlegrid::TerrainType::Mountain), 50000.0);
}

TEST(TerrainCollisionTest, MaxTerrainInRadiusReturnsType)
{
    // 5×5 map: mostly land, one mountain cell
    battlegrid::TerrainMap map(5, 5, battlegrid::TerrainType::Land);
    map.set(3, 3, battlegrid::TerrainType::Mountain);

    // Sample at (3.0, 3.0) with radius 1.0 should find the mountain
    auto sample = map.maxTerrainInRadius(3.0, 3.0, 1.0);
    EXPECT_EQ(sample.type, battlegrid::TerrainType::Mountain);
    EXPECT_DOUBLE_EQ(sample.height, 5.0);

    // Sample far from mountain should find only land
    auto sample2 = map.maxTerrainInRadius(0.5, 0.5, 0.5);
    EXPECT_EQ(sample2.type, battlegrid::TerrainType::Land);
    EXPECT_DOUBLE_EQ(sample2.height, 0.0);
}
