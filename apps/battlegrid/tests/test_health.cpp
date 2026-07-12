#include "soldier.hpp"
#include "civilian.hpp"
#include "vehicle.hpp"
#include "terrain.hpp"
#include "battlegrid_world.hpp"
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

TEST(BattleGridPhysicsTest, DynamicBodyAppliesSolvedTransform)
{
    BattleGridWorld world;
    world.loadMap(TerrainMap(16, 16, TerrainType::Land));
    InputMap input;
    world.populate(input);

    Soldier& player = world.playerSoldier();
    const COORD start = player.location();
    ASSERT_EQ(world.physicsWorld().body(player.name())->motion,
              grid::physics::BodyMotion::Dynamic);

    player.setMovementVelocity(6.0, 0.0);
    auto positions = world.engine().snapshotAgentPositions();
    world.stepCollisions(0.1, positions);

    const auto solved = world.physicsWorld().simulatedBodyPosition(player.name());
    ASSERT_TRUE(solved.has_value());
    EXPECT_GT((*solved)[0], start[0]);
    EXPECT_DOUBLE_EQ(player.location()[0], (*solved)[0]);
    EXPECT_DOUBLE_EQ(positions.at(player.name())[0], (*solved)[0]);
}

TEST(BattleGridPhysicsTest, MountedDriverUsesVehicleTransformUntilDismount)
{
    BattleGridWorld world;
    world.loadMap(TerrainMap(16, 16, TerrainType::Land));
    InputMap input;
    world.populate(input);

    Vehicle* vehicle = nullptr;
    for (const auto& agent : world.getAllAgents()) {
        vehicle = dynamic_cast<Vehicle*>(agent.get());
        if (vehicle) break;
    }
    ASSERT_NE(vehicle, nullptr);

    Soldier& player = world.playerSoldier();
    ASSERT_TRUE(world.mountSoldier(player, *vehicle));
    EXPECT_EQ(world.physicsWorld().body(player.name()), nullptr);

    auto positions = world.engine().snapshotAgentPositions();
    world.stepCollisions(0.1, positions);
    EXPECT_EQ(player.location(), vehicle->location());

    world.dismountSoldier(player, *vehicle);
    ASSERT_NE(world.physicsWorld().body(player.name()), nullptr);
}
