#include "battlegrid_world.hpp"

#include "libmap/ascii_format.hpp"
#include "libmap/map_tile.hpp"
#include "libphysics/collision_event.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <numbers>

namespace battlegrid {

/// Build a MapWorld from a loaded TerrainMap (bridge between legacy and rich types).
static grid::libmap::MapWorld terrainMapToMapWorld(const battlegrid::TerrainMap& terrain)
{
    using grid::libmap::MapWorld;
    using grid::libmap::TerrainTile;
    using grid::libmap::SurfaceType;

    MapWorld world(terrain.width(), terrain.height(), 1.0);
    for (size_t z = 0; z < terrain.height(); ++z) {
        for (size_t x = 0; x < terrain.width(); ++x) {
            TerrainTile tile;
            switch (terrain.at(x, z)) {
            case battlegrid::TerrainType::Water:    tile.surface = SurfaceType::Water;    break;
            case battlegrid::TerrainType::Mountain: tile.surface = SurfaceType::Mountain; break;
            default:                                tile.surface = SurfaceType::Land;     break;
            }
            tile.elevation = static_cast<float>(terrainHeight(terrain.at(x, z)));
            world.terrain().set(x, z, tile);
        }
    }
    return world;
}

static double shapeBottomOffset(const grid::physics::CollisionBody& body)
{
    using grid::physics::CollisionShape;
    switch (body.shape) {
    case CollisionShape::Sphere:
        return body.radius;
    case CollisionShape::Capsule:
        return std::max(body.capsuleHeight, 2.0 * body.radius) * 0.5;
    case CollisionShape::Box:
        return body.boxHalfExtents[1];
    case CollisionShape::Cylinder:
        return body.radius;
    }
    return 0.0;
}

void BattleGridWorld::rebuildTerrainColliders()
{
    constexpr double terrainDepth = 100.0;
    m_physicsWorld.clearStaticBoxes();

    for (size_t z = 0; z < m_terrain.height(); ++z) {
        for (size_t x = 0; x < m_terrain.width(); ++x) {
            const std::string name = "terrain_" + std::to_string(x) + "_" + std::to_string(z);
            const grid::physics::Vec3 center{
                static_cast<double>(x) + 0.5, 0.0, static_cast<double>(z) + 0.5};
            const auto terrain = m_terrain.at(x, z);
            switch (terrain) {
            case TerrainType::SlopeNorth:
                m_physicsWorld.addStaticRamp(
                    name, center, {0.5, terrainDepth, 0.5}, 0.0, 0.5,
                    grid::physics::RampDirection::North);
                break;
            case TerrainType::SlopeSouth:
                m_physicsWorld.addStaticRamp(
                    name, center, {0.5, terrainDepth, 0.5}, 0.0, 0.5,
                    grid::physics::RampDirection::South);
                break;
            case TerrainType::SlopeEast:
                m_physicsWorld.addStaticRamp(
                    name, center, {0.5, terrainDepth, 0.5}, 0.0, 0.5,
                    grid::physics::RampDirection::East);
                break;
            case TerrainType::SlopeWest:
                m_physicsWorld.addStaticRamp(
                    name, center, {0.5, terrainDepth, 0.5}, 0.0, 0.5,
                    grid::physics::RampDirection::West);
                break;
            default: {
                const double height = terrainHeight(terrain);
                m_physicsWorld.addStaticBox(
                    name, {center[0], height - terrainDepth, center[2]},
                    {0.5, terrainDepth, 0.5});
                break;
            }
            }
        }

        // Keep dynamic actors inside the finite terrain collider field. Without
        // these walls, wandering agents can leave the map and fall indefinitely.
        const double width = static_cast<double>(m_terrain.width());
        const double height = static_cast<double>(m_terrain.height());
        m_physicsWorld.addStaticBox("terrain_boundary_north", {width * 0.5, 0.0, -0.5},
                                    {width * 0.5 + 0.5, terrainDepth, 0.5});
        m_physicsWorld.addStaticBox("terrain_boundary_south", {width * 0.5, 0.0, height + 0.5},
                                    {width * 0.5 + 0.5, terrainDepth, 0.5});
        m_physicsWorld.addStaticBox("terrain_boundary_west", {-0.5, 0.0, height * 0.5},
                                    {0.5, terrainDepth, height * 0.5 + 0.5});
        m_physicsWorld.addStaticBox("terrain_boundary_east", {width + 0.5, 0.0, height * 0.5},
                                    {0.5, terrainDepth, height * 0.5 + 0.5});
    }
}

BattleGridWorld::BattleGridWorld()
    : m_engine(60)
{
    m_engine.getGameLog().setLogFile("battlegrid.log");
    m_physicsWorld.setGravity({0.0, -20.0, 0.0});
}

bool BattleGridWorld::loadMap(const std::string& mapPath)
{
    if (!m_terrain.loadFromFile(mapPath)) {
        std::cerr << "BattleGridWorld: failed to load map from '" << mapPath << "'\n";
        return false;
    }

    // Also try to load the rich MapWorld from the same ASCII file.
    grid::libmap::AsciiMapFormat fmt;
    auto maybeWorld = fmt.readFromFile(mapPath);
    m_mapWorld = maybeWorld ? std::move(*maybeWorld) : terrainMapToMapWorld(m_terrain);
    for (size_t z = 0; z < m_terrain.height(); ++z) {
        for (size_t x = 0; x < m_terrain.width(); ++x) {
            auto tile = m_mapWorld.terrain().get(x, z);
            tile.elevation = static_cast<float>(terrainHeight(m_terrain.at(x, z)));
            m_mapWorld.terrain().set(x, z, tile);
        }
    }
    rebuildTerrainColliders();

    std::cout << "Map loaded: " << m_terrain.width() << "x" << m_terrain.height() << "\n";
    return true;
}

void BattleGridWorld::loadMap(TerrainMap terrain)
{
    m_terrain  = std::move(terrain);
    m_mapWorld = terrainMapToMapWorld(m_terrain);
    rebuildTerrainColliders();
    std::cout << "Map loaded: " << m_terrain.width() << "x" << m_terrain.height() << "\n";
}

void BattleGridWorld::populate(InputMap& inputMap)
{
    // ── Player soldier ──────────────────────────────────────────────
    COORD playerSpawn = findSpawnPoint(TerrainType::Land);
    m_playerSoldier = std::make_shared<Soldier>(
        m_engine, playerSpawn, "Player", Faction::Blue, m_terrain, 6.0, 40.0f);
    m_playerSoldier->setPlayerControlled(true);
    m_engine.addAgent(m_playerSoldier);
    m_soldiers.push_back(m_playerSoldier);

    m_playerController = std::make_unique<PlayerController>(*m_playerSoldier, m_terrain, *this, inputMap);

    // ── Blue soldiers ───────────────────────────────────────────────
    for (int i = 0; i < 3; ++i) {
        COORD spawn = findSpawnPoint(TerrainType::Land);
        auto soldier = std::make_shared<Soldier>(
            m_engine, spawn, "BlueTeam_" + std::to_string(i + 1),
            Faction::Blue, m_terrain, 4.5);
        m_engine.addAgent(soldier);
        m_soldiers.push_back(soldier);
    }

    // ── Red soldiers ────────────────────────────────────────────────
    for (int i = 0; i < 4; ++i) {
        COORD spawn = findSpawnPoint(TerrainType::Land);
        auto soldier = std::make_shared<Soldier>(
            m_engine, spawn, "RedTeam_" + std::to_string(i + 1),
            Faction::Red, m_terrain, 4.0);
        m_engine.addAgent(soldier);
        m_soldiers.push_back(soldier);
    }

    // ── Civilians ───────────────────────────────────────────────────
    for (int i = 0; i < 5; ++i) {
        COORD spawn = findSpawnPoint(TerrainType::Land);
        auto civ = std::make_shared<Civilian>(
            m_engine, spawn, "Civilian_" + std::to_string(i + 1),
            m_terrain, 2.5);
        m_engine.addAgent(civ);
        m_civilians.push_back(civ);
    }

    // ── Land vehicles ───────────────────────────────────────────────
    for (int i = 0; i < 2; ++i) {
        COORD spawn = findSpawnPoint(TerrainType::Land);
        auto v = std::make_shared<LandVehicle>(
            m_engine, spawn, "Humvee_" + std::to_string(i + 1),
            Faction::Blue, m_terrain, 12.0);
        m_engine.addAgent(v);
        m_landVehicles.push_back(v);
    }

    // ── Sea vehicles ────────────────────────────────────────────────
    for (int i = 0; i < 2; ++i) {
        COORD spawn = findSpawnPoint(TerrainType::Water);
        auto v = std::make_shared<SeaVehicle>(
            m_engine, spawn, "Patrol_Boat_" + std::to_string(i + 1),
            Faction::Blue, m_terrain, 14.0);
        m_engine.addAgent(v);
        m_seaVehicles.push_back(v);
    }

    // ── Air vehicles ────────────────────────────────────────────────
    {
        COORD spawn = findSpawnPoint(TerrainType::Land);
        auto v = std::make_shared<AirVehicle>(
            m_engine, spawn, "Helicopter_1",
            Faction::Blue, m_terrain, 22.0, 20.0);
        m_engine.addAgent(v);
        m_airVehicles.push_back(v);
    }
    {
        COORD spawn = findSpawnPoint(TerrainType::Land);
        auto v = std::make_shared<AirVehicle>(
            m_engine, spawn, "Jet_1",
            Faction::Red, m_terrain, 40.0, 30.0);
        m_engine.addAgent(v);
        m_airVehicles.push_back(v);
    }

    std::cout << "World populated with "
              << m_soldiers.size() << " soldiers, "
              << m_civilians.size() << " civilians, "
              << m_landVehicles.size() << " land vehicles, "
              << m_seaVehicles.size() << " sea vehicles, "
              << m_airVehicles.size() << " air vehicles.\n";

    // ── Register collision bodies ───────────────────────────────────
    for (auto& s : m_soldiers)
        registerCollisionBody(s->name(), s->location(), Soldier::kMass, Soldier::kCollisionRadius,
                              grid::physics::CollisionShape::Capsule, 1.8,
                              {0.5, 0.5, 0.5}, 1.0, false, true);
    for (auto& c : m_civilians)
        registerCollisionBody(c->name(), c->location(), Civilian::kMass, Civilian::kCollisionRadius,
                              grid::physics::CollisionShape::Capsule, 1.6,
                              {0.5, 0.5, 0.5}, 1.0, false, true);
    for (auto& v : m_landVehicles)
    {
        COORD chassisPosition = v->location();
        chassisPosition[1] += LandVehicle::kChassisCenterHeight - LandVehicle::kChassisHalfHeight;
        registerCollisionBody(v->name(), chassisPosition, Vehicle::kMass, Vehicle::kCollisionRadius,
                              grid::physics::CollisionShape::Box, 1.0,
                              {0.6, LandVehicle::kChassisHalfHeight, 1.1}, 1.0, false, true, false,
                              0.3, true);
        registerLandVehicleWheels(*v);
    }
    for (auto& v : m_seaVehicles)
        registerCollisionBody(v->name(), v->location(), Vehicle::kMass, Vehicle::kCollisionRadius,
                              grid::physics::CollisionShape::Box, 1.0,
                              {0.4, 0.3, 1.0}, 1.0, false, true, false, 0.3, true);
    for (auto& v : m_airVehicles)
        registerCollisionBody(v->name(), v->location(), Vehicle::kMass, Vehicle::kCollisionRadius,
                              grid::physics::CollisionShape::Sphere, 1.0,
                              {0.5, 0.5, 0.5}, v->gravityScale());
}

void BattleGridWorld::registerLandVehicleWheels(const LandVehicle& vehicle)
{
    constexpr double wheelMass = 45.0;
    constexpr std::array<grid::physics::Vec3, LandVehicle::kWheelCount> anchors{{
        {-LandVehicle::kWheelHalfTrack, LandVehicle::kWheelAxleHeight - LandVehicle::kChassisCenterHeight,
         LandVehicle::kWheelAxleOffset},
        {LandVehicle::kWheelHalfTrack, LandVehicle::kWheelAxleHeight - LandVehicle::kChassisCenterHeight,
         LandVehicle::kWheelAxleOffset},
        {-LandVehicle::kWheelHalfTrack, LandVehicle::kWheelAxleHeight - LandVehicle::kChassisCenterHeight,
         -LandVehicle::kWheelAxleOffset},
        {LandVehicle::kWheelHalfTrack, LandVehicle::kWheelAxleHeight - LandVehicle::kChassisCenterHeight,
         -LandVehicle::kWheelAxleOffset},
    }};

    for (std::size_t index = 0; index < anchors.size(); ++index) {
        const auto& anchor = anchors[index];
        COORD wheelPosition{
            vehicle.location()[0] + anchor[0],
            vehicle.location()[1] + LandVehicle::kChassisCenterHeight + anchor[1]
                - LandVehicle::kWheelRadius,
            vehicle.location()[2] + anchor[2]};
        registerCollisionBody(vehicle.wheelName(index), wheelPosition, wheelMass, LandVehicle::kWheelRadius,
                              grid::physics::CollisionShape::Cylinder, 1.0,
                              {0.5, 0.5, 0.5}, 1.0, false, false, false, LandVehicle::kWheelWidth);
        grid::physics::WheelJoint joint;
        joint.name = vehicle.wheelName(index) + "_suspension";
        joint.chassisName = vehicle.name();
        joint.wheelName = vehicle.wheelName(index);
        joint.chassisAnchor = anchor;
        joint.steering = index < 2;
        joint.suspensionHertz = LandVehicle::kSuspensionHertz;
        joint.suspensionDampingRatio = LandVehicle::kSuspensionDampingRatio;
        joint.suspensionTravel = LandVehicle::kSuspensionTravel;
        joint.maxDriveTorque = 3000.0;
        joint.maxSteeringTorque = 1600.0;
        joint.steeringLimit = 0.50;
        m_physicsWorld.addWheelJoint(joint);
    }
}

void BattleGridWorld::removeLandVehicleWheels(const LandVehicle& vehicle)
{
    for (std::size_t index = 0; index < LandVehicle::kWheelCount; ++index) {
        const std::string wheel = vehicle.wheelName(index);
        m_physicsWorld.removeWheelJoint(wheel + "_suspension");
        m_physicsWorld.removeBody(wheel);
    }
}

void BattleGridWorld::retireDestroyedLandVehicles()
{
    for (const auto& vehicle : m_landVehicles) {
        if (!vehicle->isDead() || !m_physicsWorld.body(vehicle->name())) {
            continue;
        }
        Soldier* driver = vehicle->driver();
        if (driver) {
            driver->set_location(vehicle->location());
            driver->setYaw(vehicle->yaw());
        }
        vehicle->dismount();
        removeLandVehicleWheels(*vehicle);
        m_physicsWorld.removeBody(vehicle->name());
        if (driver) {
            registerCollisionBody(driver->name(), driver->location(), Soldier::kMass,
                                  Soldier::kCollisionRadius,
                                  grid::physics::CollisionShape::Capsule, 1.8,
                                  {0.5, 0.5, 0.5}, 1.0, false, true);
        }
    }
}

std::shared_ptr<Soldier> BattleGridWorld::addRemoteSoldier(const std::string& name)
{
    COORD spawn = findSpawnPoint(TerrainType::Land);
    auto soldier = std::make_shared<Soldier>(
        m_engine, spawn, name, Faction::Blue, m_terrain, 6.0, 40.0f);
    soldier->setPlayerControlled(true);
    m_engine.addAgent(soldier);
    m_soldiers.push_back(soldier);
    registerCollisionBody(name, spawn, Soldier::kMass, Soldier::kCollisionRadius,
                          grid::physics::CollisionShape::Capsule, 1.8,
                          {0.5, 0.5, 0.5}, 1.0, false, true);

    std::cout << "Added remote soldier: " << name << "\n";
    return soldier;
}

void BattleGridWorld::addAgentFromSnapshot(const grid::net::AgentSnapshot& snap)
{
    COORD pos{snap.position[0], snap.position[1], snap.position[2]};
    auto type = static_cast<EntityType>(snap.entityType);
    auto faction = static_cast<Faction>(snap.faction);

    switch (type) {
    case EntityType::Soldier: {
        auto s = std::make_shared<Soldier>(
            m_engine, pos, snap.name, faction, m_terrain, 4.5);
        m_engine.addAgent(s);
        m_soldiers.push_back(s);
        registerCollisionBody(snap.name, pos, Soldier::kMass, Soldier::kCollisionRadius,
                              grid::physics::CollisionShape::Capsule, 1.8,
                              {0.5, 0.5, 0.5}, 1.0, false, true);
        break;
    }
    case EntityType::Civilian: {
        auto c = std::make_shared<Civilian>(
            m_engine, pos, snap.name, m_terrain, 2.5);
        m_engine.addAgent(c);
        m_civilians.push_back(c);
        registerCollisionBody(snap.name, pos, Civilian::kMass, Civilian::kCollisionRadius,
                              grid::physics::CollisionShape::Capsule, 1.6,
                              {0.5, 0.5, 0.5}, 1.0, false, true);
        break;
    }
    case EntityType::LandVehicle: {
        auto v = std::make_shared<LandVehicle>(
            m_engine, pos, snap.name, faction, m_terrain, 12.0);
        v->setPhysicsYaw(snap.yaw);
        v->setYaw(snap.yaw);
        m_engine.addAgent(v);
        m_landVehicles.push_back(v);
        COORD chassisPosition = pos;
        chassisPosition[1] += LandVehicle::kChassisCenterHeight - LandVehicle::kChassisHalfHeight;
        registerCollisionBody(snap.name, chassisPosition, Vehicle::kMass, Vehicle::kCollisionRadius,
                              grid::physics::CollisionShape::Box, 1.0,
                              {0.6, LandVehicle::kChassisHalfHeight, 1.1}, 1.0, false, true, false,
                              0.3, true);
        registerLandVehicleWheels(*v);
        break;
    }
    case EntityType::SeaVehicle: {
        auto v = std::make_shared<SeaVehicle>(
            m_engine, pos, snap.name, faction, m_terrain, 14.0);
        v->setPhysicsYaw(snap.yaw);
        v->setYaw(snap.yaw);
        m_engine.addAgent(v);
        m_seaVehicles.push_back(v);
        registerCollisionBody(snap.name, pos, Vehicle::kMass, Vehicle::kCollisionRadius,
                              grid::physics::CollisionShape::Box, 1.0,
                              {0.4, 0.3, 1.0}, 1.0, false, true, false, 0.3, true);
        break;
    }
    case EntityType::AirVehicle: {
        auto v = std::make_shared<AirVehicle>(
            m_engine, pos, snap.name, faction, m_terrain, 22.0, 20.0);
        v->setPhysicsYaw(snap.yaw);
        v->setYaw(snap.yaw);
        m_engine.addAgent(v);
        m_airVehicles.push_back(v);
        registerCollisionBody(snap.name, pos, Vehicle::kMass, Vehicle::kCollisionRadius,
                              grid::physics::CollisionShape::Sphere, 1.0,
                              {0.5, 0.5, 0.5}, v->gravityScale());
        break;
    }
    }
    std::cout << "Added compute agent: " << snap.name << "\n";
}

bool BattleGridWorld::setAgentRemoteOwned(const std::string& name, bool owned)
{
    for (auto& s : m_soldiers) {
        if (s->name() == name) {
            s->setRemoteOwned(owned);
            if (owned) {
                m_physicsWorld.removeBody(name);
                registerCollisionBody(name, s->location(), Soldier::kMass, Soldier::kCollisionRadius,
                                      grid::physics::CollisionShape::Capsule, 1.8,
                                      {0.5, 0.5, 0.5}, 1.0, false, true, true, 0.3, false,
                                      grid::physics::BodyMotion::Kinematic);
            } else if (!m_physicsWorld.body(name)) {
                registerCollisionBody(name, s->location(), Soldier::kMass, Soldier::kCollisionRadius,
                                      grid::physics::CollisionShape::Capsule, 1.8,
                                      {0.5, 0.5, 0.5}, 1.0, false, true);
            }
            return true;
        }
    }
    for (auto& c : m_civilians) {
        if (c->name() == name) {
            c->setRemoteOwned(owned);
            if (owned) {
                m_physicsWorld.removeBody(name);
                registerCollisionBody(name, c->location(), Civilian::kMass, Civilian::kCollisionRadius,
                                      grid::physics::CollisionShape::Capsule, 1.6,
                                      {0.5, 0.5, 0.5}, 1.0, false, true, true, 0.3, false,
                                      grid::physics::BodyMotion::Kinematic);
            } else if (!m_physicsWorld.body(name)) {
                registerCollisionBody(name, c->location(), Civilian::kMass, Civilian::kCollisionRadius,
                                      grid::physics::CollisionShape::Capsule, 1.6,
                                      {0.5, 0.5, 0.5}, 1.0, false, true);
            }
            return true;
        }
    }
    for (auto& v : m_landVehicles) {
        if (v->name() == name) {
            v->setRemoteOwned(owned);
            if (owned) {
                removeLandVehicleWheels(*v);
                m_physicsWorld.removeBody(name);
                COORD chassisPosition = v->location();
                chassisPosition[1] += LandVehicle::kChassisCenterHeight - LandVehicle::kChassisHalfHeight;
                registerCollisionBody(name, chassisPosition, Vehicle::kMass, Vehicle::kCollisionRadius,
                                      grid::physics::CollisionShape::Box, 1.0,
                                      {0.6, LandVehicle::kChassisHalfHeight, 1.1}, 1.0, false, true, false,
                                      0.3, false, grid::physics::BodyMotion::Kinematic);
            } else if (!m_physicsWorld.body(name)) {
                COORD chassisPosition = v->location();
                chassisPosition[1] += LandVehicle::kChassisCenterHeight - LandVehicle::kChassisHalfHeight;
                registerCollisionBody(name, chassisPosition, Vehicle::kMass, Vehicle::kCollisionRadius,
                                      grid::physics::CollisionShape::Box, 1.0,
                                      {0.6, LandVehicle::kChassisHalfHeight, 1.1}, 1.0, false, true, false,
                                      0.3, true);
                registerLandVehicleWheels(*v);
            }
            return true;
        }
    }
    for (auto& v : m_seaVehicles) {
        if (v->name() == name) {
            v->setRemoteOwned(owned);
            if (owned) {
                m_physicsWorld.removeBody(name);
                registerCollisionBody(name, v->location(), Vehicle::kMass, Vehicle::kCollisionRadius,
                                      grid::physics::CollisionShape::Box, 1.0,
                                      {0.4, 0.3, 1.0}, 1.0, false, true, false, 0.3, false,
                                      grid::physics::BodyMotion::Kinematic);
            } else if (!m_physicsWorld.body(name)) {
                registerCollisionBody(name, v->location(), Vehicle::kMass, Vehicle::kCollisionRadius,
                                      grid::physics::CollisionShape::Box, 1.0,
                                      {0.4, 0.3, 1.0}, 1.0, false, true, false, 0.3, true);
            }
            return true;
        }
    }
    for (auto& v : m_airVehicles) {
        if (v->name() == name) {
            v->setRemoteOwned(owned);
            if (owned) {
                m_physicsWorld.removeBody(name);
                registerCollisionBody(name, v->location(), Vehicle::kMass, Vehicle::kCollisionRadius,
                                      grid::physics::CollisionShape::Sphere, 1.0,
                                      {0.5, 0.5, 0.5}, 0.0, false, true, true, 0.3, false,
                                      grid::physics::BodyMotion::Kinematic);
            } else if (!m_physicsWorld.body(name)) {
                registerCollisionBody(name, v->location(), Vehicle::kMass, Vehicle::kCollisionRadius,
                                      grid::physics::CollisionShape::Sphere, 1.0,
                                      {0.5, 0.5, 0.5}, v->gravityScale());
            }
            return true;
        }
    }
    return false;
}

bool BattleGridWorld::applyCollisionCorrection(
    const std::string& name, const grid::physics::Vec3& impulse)
{
    if (!m_physicsWorld.body(name)) {
        return false;
    }
    auto& pending = m_pendingCollisionImpulses[name];
    pending[0] += impulse[0];
    pending[1] += impulse[1];
    pending[2] += impulse[2];
    return true;
}

void BattleGridWorld::updateFromSnapshots(
    const std::vector<grid::net::AgentSnapshot>& snapshots,
    std::unordered_map<std::string, COORD>* presentationPositions)
{
    const auto syncRemoteBody = [&](const auto& actor, const grid::net::AgentSnapshot& snap,
                                    double rootOffset = 0.0) {
        if (!actor->isRemoteOwned()) {
            return;
        }
        if (const auto* body = m_physicsWorld.body(actor->name())) {
            m_physicsWorld.updateBodyPosition(
                actor->name(), snap.position[0],
                snap.position[1] + shapeBottomOffset(*body) + rootOffset, snap.position[2]);
        }
    };

    for (auto& snap : snapshots) {
        bool found = false;

        // Try to update existing soldiers
        for (auto& s : m_soldiers) {
            if (s->name() == snap.name) {
                s->set_location(COORD{snap.position[0], snap.position[1], snap.position[2]});
                s->setYaw(snap.yaw);
                s->setHealth(snap.health);
                syncRemoteBody(s, snap);
                found = true;
                break;
            }
        }
        if (found) continue;

        // Try civilians
        for (auto& c : m_civilians) {
            if (c->name() == snap.name) {
                c->set_location(COORD{snap.position[0], snap.position[1], snap.position[2]});
                c->setYaw(snap.yaw);
                c->setHealth(snap.health);
                syncRemoteBody(c, snap);
                found = true;
                break;
            }
        }
        if (found) continue;

        // Try vehicles
        auto updateVehicle = [&](auto& list, double rootOffset = 0.0) -> bool {
            for (auto& v : list) {
                if (v->name() == snap.name) {
                    v->set_location(COORD{snap.position[0], snap.position[1], snap.position[2]});
                    v->setPhysicsYaw(snap.yaw);
                    v->setYaw(snap.yaw);
                    v->setHealth(snap.health);
                    syncRemoteBody(v, snap, rootOffset);
                    return true;
                }
            }
            return false;
        };
        if (updateVehicle(
                m_landVehicles, LandVehicle::kChassisCenterHeight - LandVehicle::kChassisHalfHeight)) {
            if (presentationPositions && snap.hasWheelPresentation) {
                for (const auto& vehicle : m_landVehicles) {
                    if (vehicle->name() != snap.name) {
                        continue;
                    }
                    for (std::size_t index = 0; index < LandVehicle::kWheelCount; ++index) {
                        const auto& wheel = snap.wheelPositions[index];
                        (*presentationPositions)[vehicle->wheelName(index)] = {
                            wheel[0], wheel[1], wheel[2]};
                    }
                    break;
                }
            }
            continue;
        }
        if (updateVehicle(m_seaVehicles)) continue;
        if (updateVehicle(m_airVehicles)) continue;

        // Agent not found locally — create it
        addAgentFromSnapshot(snap);
    }
}

void BattleGridWorld::registerCollisionBody(const std::string& name, const COORD& pos,
                                            double mass, double radius,
                                            grid::physics::CollisionShape shape,
                                            double capsuleHeight,
                                            grid::physics::Vec3 boxHalfExtents,
                                            double gravityScale,
                                            bool lockVerticalMotion, bool lockRotation,
                                            bool lockYawRotation, double cylinderHeight, bool isBullet,
                                            grid::physics::BodyMotion motion)
{
    grid::physics::CollisionBody body;
    body.name = name;
    body.mass = mass;
    body.radius = radius;
    body.shape = shape;
    body.capsuleHeight = capsuleHeight;
    body.boxHalfExtents = boxHalfExtents;
    body.cylinderHeight = cylinderHeight;
    body.position = {
        pos[0],
        pos[1] + (gravityScale > 0.0 || lockVerticalMotion ? shapeBottomOffset(body) : 0.0),
        pos[2]};
    body.prevPosition = body.position;
    body.motion = motion;
    body.gravityScale = gravityScale;
    body.lockVerticalMotion = lockVerticalMotion;
    body.lockRotation = lockRotation;
    body.lockYawRotation = lockYawRotation;
    body.isBullet = isBullet;
    m_physicsWorld.addBody(body);
}

COORD BattleGridWorld::findSpawnPoint(TerrainType required) const
{
    // Simple random search for a tile of the required terrain type
    size_t w = m_terrain.width();
    size_t h = m_terrain.height();

    for (int attempt = 0; attempt < 200; ++attempt) {
        size_t x = static_cast<size_t>(std::rand()) % w;
        size_t z = static_cast<size_t>(std::rand()) % h;
        if (m_terrain.at(x, z) == required) {
            double height = terrainHeight(required);
            return COORD{static_cast<double>(x) + 0.5,
                         height,
                         static_cast<double>(z) + 0.5};
        }
    }

    // Fallback: scan for the first matching tile
    for (size_t z = 0; z < h; ++z) {
        for (size_t x = 0; x < w; ++x) {
            if (m_terrain.at(x, z) == required) {
                double height = terrainHeight(required);
                return COORD{static_cast<double>(x) + 0.5,
                             height,
                             static_cast<double>(z) + 0.5};
            }
        }
    }

    // Absolute fallback: center of map
    return COORD{static_cast<double>(w) / 2.0, 0.0, static_cast<double>(h) / 2.0};
}

Vehicle* BattleGridWorld::findNearestVehicle(const COORD& pos, double range,
                                              const std::unordered_map<std::string, COORD>& positions) const
{
    Vehicle* best = nullptr;
    double bestDist = range;

    auto check = [&](Vehicle* v) {
        auto it = positions.find(v->name());
        if (it == positions.end()) return;
        double dx = it->second[0] - pos[0];
        double dz = it->second[2] - pos[2];
        double dist = std::sqrt(dx * dx + dz * dz);
        if (dist < bestDist) {
            bestDist = dist;
            best = v;
        }
    };
    for (auto& v : m_landVehicles) check(v.get());
    for (auto& v : m_seaVehicles)  check(v.get());
    for (auto& v : m_airVehicles)  check(v.get());

    return best;
}

bool BattleGridWorld::mountSoldier(Soldier& soldier, Vehicle& vehicle)
{
    if (!vehicle.mount(&soldier)) {
        return false;
    }
    m_physicsWorld.removeBody(soldier.name());
    return true;
}

void BattleGridWorld::dismountSoldier(Soldier& soldier, Vehicle& vehicle)
{
    const auto vehiclePosition = m_physicsWorld.simulatedBodyPosition(vehicle.name());
    vehicle.dismount();
    if (!vehiclePosition) {
        return;
    }

    double vehicleHalfWidth = Vehicle::kCollisionRadius;
    if (const auto* body = m_physicsWorld.body(vehicle.name());
        body && body->shape == grid::physics::CollisionShape::Box) {
        vehicleHalfWidth = body->boxHalfExtents[0];
    }
    // Land-vehicle tires extend farther sideways than the chassis.  Spawn
    // clear of both, along the chassis' local right axis, so the new capsule
    // cannot begin overlapped with a dynamic wheel or hull.
    if (dynamic_cast<LandVehicle*>(&vehicle)) {
        vehicleHalfWidth = std::max(vehicleHalfWidth, 0.70 + 0.20 * 0.5);
    }
    const double yaw = vehicle.yaw();
    const double dismountDistance = vehicleHalfWidth + Soldier::kCollisionRadius + 0.15;
    COORD position{vehicle.location()[0] + std::sin(yaw) * dismountDistance,
                   vehicle.location()[1],
                   vehicle.location()[2] - std::cos(yaw) * dismountDistance};
    registerCollisionBody(soldier.name(), position, Soldier::kMass,
                         Soldier::kCollisionRadius,
                         grid::physics::CollisionShape::Capsule, 1.8,
                         {0.5, 0.5, 0.5}, 1.0, false, true);
}

void BattleGridWorld::submitActorVelocities()
{
    constexpr double jumpSpeed = 8.0;
    auto submit = [&](const auto& actor) {
        const auto current = m_physicsWorld.simulatedBodyVelocity(actor->name());
        auto desired = actor->movementVelocity();
        desired[1] = current ? (*current)[1] : 0.0;
        m_physicsWorld.setSimulatedBodyVelocity(actor->name(), desired);
    };

    for (const auto& soldier : m_soldiers) {
        if (soldier->isRemoteOwned()) {
            continue;
        }
        submit(soldier);
        if (soldier->consumeJumpRequest()
            && m_physicsWorld.simulatedBodyGrounded(soldier->name())) {
            const auto horizontal = soldier->movementVelocity();
            m_physicsWorld.setSimulatedBodyVelocity(
                soldier->name(), {horizontal[0], jumpSpeed, horizontal[2]});
        }
    }
    for (const auto& civilian : m_civilians) {
        if (civilian->isRemoteOwned()) {
            continue;
        }
        submit(civilian);
        if (civilian->consumeJumpRequest()
            && m_physicsWorld.simulatedBodyGrounded(civilian->name())) {
            const auto horizontal = civilian->movementVelocity();
            m_physicsWorld.setSimulatedBodyVelocity(
                civilian->name(), {horizontal[0], jumpSpeed, horizontal[2]});
        }
    }
    for (const auto& vehicle : m_landVehicles) {
        if (vehicle->isRemoteOwned() || !m_physicsWorld.body(vehicle->name())) {
            continue;
        }
        double throttle = vehicle->throttle();
        double steering = vehicle->steering();
        if (!vehicle->hasDriver()) {
            const auto desired = vehicle->movementVelocity();
            const double desiredSpeed = std::hypot(desired[0], desired[2]);
            throttle = std::clamp(desiredSpeed / vehicle->speed(), 0.0, 1.0);
            if (desiredSpeed > 1e-6) {
                const double desiredYaw = std::atan2(desired[2], desired[0]);
                const double yawError = std::remainder(
                    desiredYaw - vehicle->yaw(), 2.0 * std::numbers::pi);
                // Physics yaw grows in the opposite direction to BattleGrid's
                // render-facing yaw, so the autonomous steering sign reverses.
                steering = -std::clamp(yawError / 0.50, -1.0, 1.0);
            }
        }
        const double spinSpeed = -throttle * vehicle->speed() / LandVehicle::kWheelRadius;
        const double steeringAngle = steering * 0.50;
        for (std::size_t index = 0; index < LandVehicle::kWheelCount; ++index) {
            const std::string joint = vehicle->wheelName(index) + "_suspension";
            m_physicsWorld.setWheelJointDrive(joint, spinSpeed);
            if (index < 2) {
                m_physicsWorld.setWheelJointSteering(joint, steeringAngle);
            }
        }
    }
    for (const auto& vehicle : m_seaVehicles) submit(vehicle);
    for (const auto& vehicle : m_airVehicles) {
        auto desired = vehicle->movementVelocity();
        m_physicsWorld.setSimulatedBodyVelocity(vehicle->name(), desired);
    }
}

void BattleGridWorld::applySolvedTransforms(
    std::unordered_map<std::string, COORD>& positions)
{
    auto apply = [&](const auto& actor, bool locationIsGrounded, double rootOffset = 0.0) {
        const auto position = m_physicsWorld.simulatedBodyPosition(actor->name());
        if (!position) {
            return;
        }
        double bottomOffset = 0.0;
        if (locationIsGrounded) {
            if (const auto* body = m_physicsWorld.body(actor->name())) {
                bottomOffset = shapeBottomOffset(*body);
            }
        }
        const COORD solved{
            (*position)[0], (*position)[1] - bottomOffset - rootOffset, (*position)[2]};
        actor->set_location(solved);
        positions[actor->name()] = solved;
    };

    for (const auto& soldier : m_soldiers) apply(soldier, true);
    for (const auto& civilian : m_civilians) apply(civilian, true);
    for (const auto& vehicle : m_landVehicles) {
        apply(vehicle, true, LandVehicle::kChassisCenterHeight - LandVehicle::kChassisHalfHeight);
        for (std::size_t index = 0; index < LandVehicle::kWheelCount; ++index) {
            if (const auto wheel = m_physicsWorld.simulatedBodyPosition(vehicle->wheelName(index))) {
                positions[vehicle->wheelName(index)] = {(*wheel)[0], (*wheel)[1], (*wheel)[2]};
            }
        }
    }
    for (const auto& vehicle : m_seaVehicles) apply(vehicle, true);
    for (const auto& vehicle : m_airVehicles) apply(vehicle, false);

    auto applyVehicleYaw = [&](const auto& vehicle) {
        if (const auto yaw = m_physicsWorld.simulatedBodyYaw(vehicle->name())) {
            vehicle->setPhysicsYaw(std::numbers::pi / 2.0 - *yaw);
        }
    };
    for (const auto& vehicle : m_landVehicles) applyVehicleYaw(vehicle);
    for (const auto& vehicle : m_seaVehicles) applyVehicleYaw(vehicle);

    for (const auto& vehicle : m_landVehicles) {
        if (auto* driver = vehicle->driver()) {
            driver->set_location(vehicle->location());
            driver->setYaw(vehicle->yaw());
            positions[driver->name()] = driver->location();
        }
    }
    for (const auto& vehicle : m_seaVehicles) {
        if (auto* driver = vehicle->driver()) {
            driver->set_location(vehicle->location());
            driver->setYaw(vehicle->yaw());
            positions[driver->name()] = driver->location();
        }
    }
    for (const auto& vehicle : m_airVehicles) {
        if (auto* driver = vehicle->driver()) {
            driver->set_location(vehicle->location());
            driver->setYaw(vehicle->yaw());
            positions[driver->name()] = driver->location();
        }
    }
}

void BattleGridWorld::stepCollisions(double dt,
                                     std::unordered_map<std::string, COORD>& positions)
{
    retireDestroyedLandVehicles();
    submitActorVelocities();
    for (const auto& [name, impulse] : m_pendingCollisionImpulses) {
        m_physicsWorld.applySimulatedBodyImpulse(name, impulse);
    }
    m_pendingCollisionImpulses.clear();
    for (const auto& vehicle : m_seaVehicles) {
        m_physicsWorld.setSimulatedBodyYaw(vehicle->name(), vehicle->desiredYaw());
    }
    const auto collisions = m_physicsWorld.step(dt);
    applySolvedTransforms(positions);

    auto agents = m_engine.getAllAgents();
    const auto isRemoteOwned = [&](const std::string& name) {
        for (const auto& agent : agents) {
            if (agent->name() != name) {
                continue;
            }
            if (const auto* soldier = dynamic_cast<const Soldier*>(agent.get())) {
                return soldier->isRemoteOwned();
            }
            if (const auto* civilian = dynamic_cast<const Civilian*>(agent.get())) {
                return civilian->isRemoteOwned();
            }
            if (const auto* vehicle = dynamic_cast<const Vehicle*>(agent.get())) {
                return vehicle->isRemoteOwned();
            }
            return false;
        }
        return false;
    };
    for (const auto& col : collisions) {
        // A compute-owned actor is represented here by a kinematic snapshot
        // proxy. It blocks server-owned actors, but cannot author local
        // collision gameplay because its compute owner has the real body.
        if (isRemoteOwned(col.nameA) || isRemoteOwned(col.nameB)) {
            const std::string& localName = isRemoteOwned(col.nameA) ? col.nameB : col.nameA;
            const std::string& remoteName = isRemoteOwned(col.nameA) ? col.nameA : col.nameB;
            m_engine.getGameLog().log(
                localName, "", grid::libsim::Senses::Touch,
                "Collided with " + remoteName + "!");
            if (m_collisionCorrectionHandler) {
                const bool remoteIsA = remoteName == col.nameA;
                const auto localVelocity = remoteIsA
                    ? grid::physics::Vec3{-col.relativeVelocity[0], -col.relativeVelocity[1],
                                          -col.relativeVelocity[2]}
                    : col.relativeVelocity;
                const double approachSpeed = std::max(
                    0.0, localVelocity[0] * (remoteIsA ? -col.normal[0] : col.normal[0])
                        + localVelocity[1] * (remoteIsA ? -col.normal[1] : col.normal[1])
                        + localVelocity[2] * (remoteIsA ? -col.normal[2] : col.normal[2]));
                const auto* localBody = m_physicsWorld.body(localName);
                const double impulse = std::max(col.impulse, approachSpeed * (localBody ? localBody->mass : 0.0));
                if (impulse > 0.0) {
                    const double direction = remoteIsA ? -1.0 : 1.0;
                    m_collisionCorrectionHandler(remoteName, {
                        direction * col.normal[0] * impulse,
                        direction * col.normal[1] * impulse,
                        direction * col.normal[2] * impulse});
                }
            }
            continue;
        }

        // Event for entity A (normal points toward B).
        grid::physics::CollisionEvent evtA(
            col.nameA, col.nameB, col.normal, col.relativeVelocity,
            col.impulse, col.massA, col.massB, col.penetration);

        // Event for entity B (flip normal and relative velocity).
        grid::physics::Vec3 flipNormal = {
            -col.normal[0], -col.normal[1], -col.normal[2]};
        grid::physics::Vec3 flipRelVel = {
            -col.relativeVelocity[0], -col.relativeVelocity[1],
            -col.relativeVelocity[2]};
        grid::physics::CollisionEvent evtB(
            col.nameB, col.nameA, flipNormal, flipRelVel,
            col.impulse, col.massB, col.massA, col.penetration);

        // Deliver to the agents by name.
        for (auto& agent : agents) {
            if (agent->name() == col.nameA) agent->on_event(&evtA);
            if (agent->name() == col.nameB) agent->on_event(&evtB);
        }
    }
}

} // namespace battlegrid
