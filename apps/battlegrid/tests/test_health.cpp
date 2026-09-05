#include "soldier.hpp"
#include "civilian.hpp"
#include "vehicle.hpp"
#include "terrain.hpp"
#include "battlegrid_world.hpp"
#include "libphysics/collision_event.hpp"
#include "libsim/base_engine.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <numbers>

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
    // impulse / mass = 2000 / 80 = 25.0, above threshold 20.0 → damage = 5.0
    auto evt = makeHit("S1", "Other", 2000.0, Soldier::kMass, 2000.0);
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

TEST(BattleGridPhysicsTest, GroundedPlayerCanJumpWhileMoving)
{
    BattleGridWorld world;
    world.loadMap(TerrainMap(16, 16, TerrainType::Land));
    InputMap input;
    world.populate(input);
    world.physicsWorld().setMaxSubSteps(0);

    Soldier& player = world.playerSoldier();
    auto positions = world.engine().snapshotAgentPositions();
    world.stepCollisions(1.0 / 60.0, positions);
    ASSERT_TRUE(world.physicsWorld().simulatedBodyGrounded(player.name()));

    const double startX = player.location()[0];
    player.setMovementVelocity(6.0, 0.0);
    player.jump();
    world.stepCollisions(1.0 / 60.0, positions);

    const auto velocity = world.physicsWorld().simulatedBodyVelocity(player.name());
    ASSERT_TRUE(velocity.has_value());
    EXPECT_GT((*velocity)[1], 7.0);
    EXPECT_GT(player.location()[0], startX);
}

TEST(BattleGridPhysicsTest, RegistersPeopleAsCapsulesAndSurfaceVehiclesAsHullBoxes)
{
    BattleGridWorld world;
    world.loadMap(TerrainMap(16, 16, TerrainType::Land));
    InputMap input;
    world.populate(input);

    const Soldier& player = world.playerSoldier();
    const auto* playerBody = world.physicsWorld().body(player.name());
    ASSERT_NE(playerBody, nullptr);
    EXPECT_EQ(playerBody->shape, grid::physics::CollisionShape::Capsule);
    EXPECT_DOUBLE_EQ(playerBody->radius, Soldier::kCollisionRadius);
    EXPECT_DOUBLE_EQ(playerBody->capsuleHeight, 1.8);
    EXPECT_DOUBLE_EQ(playerBody->position[1], player.location()[1] + 0.9);
    EXPECT_TRUE(playerBody->lockRotation);

    const grid::physics::CollisionBody* landBody = nullptr;
    const LandVehicle* landVehicle = nullptr;
    const grid::physics::CollisionBody* seaBody = nullptr;
    const grid::physics::CollisionBody* airBody = nullptr;
    const AirVehicle* airVehicle = nullptr;
    for (const auto& agent : world.getAllAgents()) {
        if (dynamic_cast<LandVehicle*>(agent.get())) {
            landBody = world.physicsWorld().body(agent->name());
            landVehicle = static_cast<LandVehicle*>(agent.get());
        } else if (dynamic_cast<SeaVehicle*>(agent.get())) {
            seaBody = world.physicsWorld().body(agent->name());
        } else if (auto* air = dynamic_cast<AirVehicle*>(agent.get())) {
            airBody = world.physicsWorld().body(agent->name());
            airVehicle = air;
        }
    }

    ASSERT_NE(landBody, nullptr);
    ASSERT_NE(landVehicle, nullptr);
    EXPECT_EQ(landBody->shape, grid::physics::CollisionShape::Box);
    EXPECT_EQ(landBody->boxHalfExtents,
              (grid::physics::Vec3{0.6, LandVehicle::kChassisHalfHeight, 1.1}));
    EXPECT_DOUBLE_EQ(landBody->position[1], LandVehicle::kChassisCenterHeight);
    EXPECT_TRUE(landBody->lockRotation);
    EXPECT_FALSE(landBody->lockYawRotation);
    for (std::size_t index = 0; index < LandVehicle::kWheelCount; ++index) {
        const std::string wheelName = landVehicle->wheelName(index);
        const auto* wheel = world.physicsWorld().body(wheelName);
        ASSERT_NE(wheel, nullptr);
        EXPECT_EQ(wheel->shape, grid::physics::CollisionShape::Cylinder);
        EXPECT_DOUBLE_EQ(wheel->radius, LandVehicle::kWheelRadius);
        EXPECT_DOUBLE_EQ(wheel->cylinderHeight, LandVehicle::kWheelWidth);
        EXPECT_DOUBLE_EQ(wheel->position[1], LandVehicle::kWheelAxleHeight);
        EXPECT_EQ(wheel->motion, grid::physics::BodyMotion::Dynamic);
        EXPECT_FALSE(wheel->lockRotation);

        const auto* suspension =
            world.physicsWorld().wheelJoint(wheelName + "_suspension");
        ASSERT_NE(suspension, nullptr);
        EXPECT_EQ(suspension->chassisName, landVehicle->name());
        EXPECT_EQ(suspension->wheelName, wheelName);
        EXPECT_EQ(suspension->steering, index < 2);
        EXPECT_DOUBLE_EQ(suspension->suspensionHertz, LandVehicle::kSuspensionHertz);
        EXPECT_DOUBLE_EQ(suspension->suspensionDampingRatio, LandVehicle::kSuspensionDampingRatio);
        EXPECT_DOUBLE_EQ(suspension->suspensionTravel, LandVehicle::kSuspensionTravel);
        EXPECT_DOUBLE_EQ(suspension->chassisAnchor[1],
                         LandVehicle::kWheelAxleHeight - LandVehicle::kChassisCenterHeight);
    }
    EXPECT_GT(landBody->position[1] - landBody->boxHalfExtents[1],
              LandVehicle::kWheelRadius);

    ASSERT_NE(seaBody, nullptr);
    EXPECT_EQ(seaBody->shape, grid::physics::CollisionShape::Box);
    EXPECT_EQ(seaBody->boxHalfExtents, (grid::physics::Vec3{0.4, 0.3, 1.0}));
    EXPECT_TRUE(seaBody->lockRotation);
    EXPECT_FALSE(seaBody->lockYawRotation);

    ASSERT_NE(airBody, nullptr);
    ASSERT_NE(airVehicle, nullptr);
    EXPECT_EQ(airBody->shape, grid::physics::CollisionShape::Sphere);
    EXPECT_DOUBLE_EQ(airBody->position[1], airVehicle->location()[1]);
}

TEST(BattleGridMapTest, DefaultMapContainsNavigableSuspensionBumps)
{
    const auto mapPath = std::filesystem::path(__FILE__).parent_path().parent_path()
        / "maps" / "default.map";
    TerrainMap map;
    ASSERT_TRUE(map.loadFromFile(mapPath.string()));
    ASSERT_EQ(map.width(), 64u);
    ASSERT_EQ(map.height(), 64u);

    std::size_t bumps = 0;
    for (std::size_t z = 0; z < map.height(); ++z) {
        for (std::size_t x = 0; x < map.width(); ++x) {
            if (map.at(x, z) == TerrainType::Bump) {
                ++bumps;
                EXPECT_DOUBLE_EQ(map.heightAt(static_cast<double>(x), static_cast<double>(z)),
                                 kBumpHeight);
            }
        }
    }
    EXPECT_GT(bumps, 0u);
    EXPECT_LT(terrainHeight(TerrainType::Bump), Vehicle::kMaxStepUp);

    BattleGridWorld world;
    ASSERT_TRUE(world.loadMap(mapPath.string()));
    EXPECT_EQ(world.physicsWorld().staticBoxCount(), 64u * 64u);
    for (std::size_t z = 0; z < map.height(); ++z) {
        for (std::size_t x = 0; x < map.width(); ++x) {
            if (map.at(x, z) == TerrainType::Bump) {
                const auto& tile = world.mapWorld().terrain().get(x, z);
                EXPECT_EQ(tile.surface, grid::libmap::SurfaceType::Land);
                EXPECT_FLOAT_EQ(tile.elevation, static_cast<float>(kBumpHeight));
                return;
            }
        }
    }
}

TEST(BattleGridMapTest, DefaultMapContainsDriveableSlopedHill)
{
    const auto mapPath = std::filesystem::path(__FILE__).parent_path().parent_path()
        / "maps" / "default.map";
    TerrainMap map;
    ASSERT_TRUE(map.loadFromFile(mapPath.string()));

    std::size_t slopes = 0;
    std::size_t hills = 0;
    for (std::size_t z = 0; z < map.height(); ++z) {
        for (std::size_t x = 0; x < map.width(); ++x) {
            const auto terrain = map.at(x, z);
            slopes += terrain == TerrainType::SlopeNorth || terrain == TerrainType::SlopeSouth
                || terrain == TerrainType::SlopeEast || terrain == TerrainType::SlopeWest;
            hills += terrain == TerrainType::Hill;
        }
    }
    EXPECT_EQ(slopes, 4u);
    EXPECT_EQ(hills, 1u);

    BattleGridWorld world;
    ASSERT_TRUE(world.loadMap(mapPath.string()));
    EXPECT_EQ(world.physicsWorld().staticBoxCount(), 64u * 64u);
    const auto& hill = world.mapWorld().terrain().get(32, 24);
    EXPECT_EQ(hill.surface, grid::libmap::SurfaceType::Land);
    EXPECT_FLOAT_EQ(hill.elevation, 0.5f);
    EXPECT_EQ(world.mapWorld().terrain().get(32, 23).tags.at("battlegrid:slope"), "S");
}

TEST(InputMapTest, ClearReleasesHeldInputsAndPendingActions)
{
    InputMap input;
    input.bindKey(io::Key::W, GameAction::MoveZ, -1.0f);
    input.bindKey(io::Key::E, GameAction::Interact);
    input.bindMouseX(GameAction::LookX);

    input.processEvent(io::KeyEvent{io::Key::W, io::Action::Press});
    input.processEvent(io::KeyEvent{io::Key::E, io::Action::Press});
    input.processEvent(io::MouseMoveEvent{0, 0, 12, 0});
    input.clear();

    EXPECT_FLOAT_EQ(input.axis(GameAction::MoveZ), 0.0f);
    EXPECT_FALSE(input.pressed(GameAction::Interact));
    EXPECT_FLOAT_EQ(input.delta(GameAction::LookX), 0.0f);
}

TEST(BattleGridPhysicsTest, RemoteSoldierUsesAnUprightCapsule)
{
    BattleGridWorld world;
    world.loadMap(TerrainMap(16, 16, TerrainType::Land));

    auto soldier = world.addRemoteSoldier("remote_player");
    const auto* body = world.physicsWorld().body(soldier->name());

    ASSERT_NE(body, nullptr);
    EXPECT_EQ(body->shape, grid::physics::CollisionShape::Capsule);
    EXPECT_DOUBLE_EQ(body->capsuleHeight, 1.8);
    EXPECT_TRUE(body->lockRotation);
}

TEST(BattleGridPhysicsTest, MountedLandVehicleUsesForwardReverseAndSteeringControls)
{
    BattleGridWorld world;
    world.loadMap(TerrainMap(16, 16, TerrainType::Land));
    InputMap input;
    input.bindKey(io::Key::W, GameAction::MoveZ, -1.0f);
    input.bindKey(io::Key::S, GameAction::MoveZ, 1.0f);
    input.bindKey(io::Key::A, GameAction::MoveX, -1.0f);
    input.bindKey(io::Key::E, GameAction::Interact);
    world.populate(input);

    LandVehicle* vehicle = nullptr;
    for (const auto& agent : world.getAllAgents()) {
        vehicle = dynamic_cast<LandVehicle*>(agent.get());
        if (vehicle) break;
    }
    ASSERT_NE(vehicle, nullptr);
    Soldier& player = world.playerSoldier();
    auto positions = world.engine().snapshotAgentPositions();
    positions[player.name()] = vehicle->location();
    world.playerController().setSnapshotPosition(vehicle->location());

    input.processEvent(io::KeyEvent{io::Key::E, io::Action::Press});
    world.playerController().update(1.0 / 60.0, positions);
    ASSERT_TRUE(world.playerController().inVehicle());
    input.processEvent(io::KeyEvent{io::Key::E, io::Action::Release});
    input.endFrame();

    input.processEvent(io::KeyEvent{io::Key::W, io::Action::Press});
    input.processEvent(io::KeyEvent{io::Key::A, io::Action::Press});
    world.playerController().update(1.0 / 60.0, positions);
    EXPECT_DOUBLE_EQ(vehicle->throttle(), 1.0);
    EXPECT_DOUBLE_EQ(vehicle->steering(), -1.0);
    world.stepCollisions(1.0 / 60.0, positions);
    for (std::size_t index = 0; index < LandVehicle::kWheelCount; ++index) {
        EXPECT_TRUE(positions.contains(vehicle->wheelName(index)));
    }
    const auto* frontWheel =
        world.physicsWorld().wheelJoint(vehicle->wheelName(0) + "_suspension");
    const auto* rearWheel =
        world.physicsWorld().wheelJoint(vehicle->wheelName(2) + "_suspension");
    ASSERT_NE(frontWheel, nullptr);
    ASSERT_NE(rearWheel, nullptr);
    EXPECT_LT(frontWheel->driveSpeed, 0.0);
    EXPECT_NEAR(frontWheel->targetSteeringAngle, -0.50, 1e-9);
    EXPECT_LT(rearWheel->driveSpeed, 0.0);
    EXPECT_DOUBLE_EQ(rearWheel->targetSteeringAngle, 0.0);

    const auto start = vehicle->location();
    for (int step = 0; step < 60; ++step) {
        world.stepCollisions(1.0 / 60.0, positions);
    }
    EXPECT_GT(std::hypot(vehicle->location()[0] - start[0],
                         vehicle->location()[2] - start[2]),
              0.1);

    input.processEvent(io::KeyEvent{io::Key::W, io::Action::Release});
    input.processEvent(io::KeyEvent{io::Key::A, io::Action::Release});
    input.processEvent(io::KeyEvent{io::Key::S, io::Action::Press});
    world.playerController().update(1.0 / 60.0, positions);
    EXPECT_DOUBLE_EQ(vehicle->throttle(), -1.0);
    EXPECT_DOUBLE_EQ(vehicle->steering(), 0.0);
    world.stepCollisions(1.0 / 60.0, positions);
    EXPECT_GT(frontWheel->driveSpeed, 0.0);
}

TEST(BattleGridPhysicsTest, PlayerCanJumpFromDynamicLandVehicleRoof)
{
    BattleGridWorld world;
    world.loadMap(TerrainMap(16, 16, TerrainType::Land));
    InputMap input;
    world.populate(input);
    world.physicsWorld().setMaxSubSteps(0);

    LandVehicle* vehicle = nullptr;
    for (const auto& agent : world.getAllAgents()) {
        vehicle = dynamic_cast<LandVehicle*>(agent.get());
        if (vehicle) break;
    }
    ASSERT_NE(vehicle, nullptr);

    const auto chassis = world.physicsWorld().simulatedBodyPosition(vehicle->name());
    ASSERT_TRUE(chassis.has_value());
    Soldier& player = world.playerSoldier();
    world.physicsWorld().updateBodyPosition(
        player.name(), (*chassis)[0],
        (*chassis)[1] + 0.4 + 0.9 - 0.02, (*chassis)[2]);
    world.physicsWorld().setSimulatedBodyVelocity(player.name(), {0.0, 0.0, 0.0});

    auto positions = world.engine().snapshotAgentPositions();
    world.stepCollisions(1.0 / 120.0, positions);
    ASSERT_TRUE(world.physicsWorld().simulatedBodyGrounded(player.name()));

    player.setMovementVelocity(4.0, 0.0);
    player.jump();
    world.stepCollisions(1.0 / 120.0, positions);

    const auto velocity = world.physicsWorld().simulatedBodyVelocity(player.name());
    ASSERT_TRUE(velocity.has_value());
    EXPECT_GT((*velocity)[1], 7.0);
    EXPECT_GT((*velocity)[0], 0.0);
}

TEST(BattleGridPhysicsTest, DestroyedLandVehicleCleansUpWheelsAndMountedDriver)
{
    BattleGridWorld world;
    world.loadMap(TerrainMap(16, 16, TerrainType::Land));
    InputMap input;
    world.populate(input);

    LandVehicle* vehicle = nullptr;
    for (const auto& agent : world.getAllAgents()) {
        vehicle = dynamic_cast<LandVehicle*>(agent.get());
        if (vehicle) break;
    }
    ASSERT_NE(vehicle, nullptr);
    Soldier& player = world.playerSoldier();
    ASSERT_TRUE(world.mountSoldier(player, *vehicle));
    vehicle->takeDamage(Vehicle::kMaxHealth);

    auto positions = world.engine().snapshotAgentPositions();
    world.stepCollisions(1.0 / 60.0, positions);
    EXPECT_EQ(world.physicsWorld().body(vehicle->name()), nullptr);
    EXPECT_FALSE(vehicle->hasDriver());
    ASSERT_NE(world.physicsWorld().body(player.name()), nullptr);
    for (std::size_t index = 0; index < LandVehicle::kWheelCount; ++index) {
        EXPECT_EQ(world.physicsWorld().body(vehicle->wheelName(index)), nullptr);
        EXPECT_EQ(world.physicsWorld().wheelJoint(
                      vehicle->wheelName(index) + "_suspension"),
                  nullptr);
    }
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
    const auto chassis = world.physicsWorld().simulatedBodyPosition(vehicle->name());
    const auto dismounted = world.physicsWorld().simulatedBodyPosition(player.name());
    ASSERT_TRUE(chassis.has_value());
    ASSERT_TRUE(dismounted.has_value());
    EXPECT_GT(std::hypot((*dismounted)[0] - (*chassis)[0],
                         (*dismounted)[2] - (*chassis)[2]),
              0.70 + 0.20 * 0.5 + Soldier::kCollisionRadius);
}
