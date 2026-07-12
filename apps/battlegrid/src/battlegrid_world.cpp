#include "battlegrid_world.hpp"

#include "libmap/ascii_format.hpp"
#include "libmap/map_tile.hpp"
#include "libphysics/collision_event.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <algorithm>

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
            world.terrain().set(x, z, tile);
        }
    }
    return world;
}

void BattleGridWorld::rebuildTerrainColliders()
{
    constexpr double terrainDepth = 100.0;
    m_physicsWorld.clearStaticBoxes();

    for (size_t z = 0; z < m_terrain.height(); ++z) {
        for (size_t x = 0; x < m_terrain.width(); ++x) {
            const double height = terrainHeight(m_terrain.at(x, z));
            m_physicsWorld.addStaticBox(
                "terrain_" + std::to_string(x) + "_" + std::to_string(z),
                {static_cast<double>(x) + 0.5, height - terrainDepth, static_cast<double>(z) + 0.5},
                {0.5, terrainDepth, 0.5});
        }
    }
}

BattleGridWorld::BattleGridWorld()
    : m_engine(60)
{
    m_engine.getGameLog().setLogFile("battlegrid.log");
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
        registerCollisionBody(s->name(), s->location(), Soldier::kMass, Soldier::kCollisionRadius);
    for (auto& c : m_civilians)
        registerCollisionBody(c->name(), c->location(), Civilian::kMass, Civilian::kCollisionRadius);
    for (auto& v : m_landVehicles)
        registerCollisionBody(v->name(), v->location(), Vehicle::kMass, Vehicle::kCollisionRadius);
    for (auto& v : m_seaVehicles)
        registerCollisionBody(v->name(), v->location(), Vehicle::kMass, Vehicle::kCollisionRadius);
    for (auto& v : m_airVehicles)
        registerCollisionBody(v->name(), v->location(), Vehicle::kMass, Vehicle::kCollisionRadius);
}

void BattleGridWorld::registerCollisionBody(const std::string& name, const COORD& pos,
                                            double mass, double radius)
{
    grid::physics::CollisionBody body;
    body.name = name;
    body.position = {pos[0], pos[1], pos[2]};
    body.prevPosition = body.position;
    body.mass = mass;
    body.radius = radius;
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

void BattleGridWorld::stepCollisions(double dt,
                                     const std::unordered_map<std::string, COORD>& positions)
{
    // Sync entity positions into the physics world.
    for (const auto& [name, pos] : positions) {
        m_physicsWorld.updateBodyPosition(name, pos[0], pos[1], pos[2]);
    }

    // Run fixed-timestep collision detection + impulse resolution.
    auto collisions = m_physicsWorld.step(dt);

    // Apply positional separation and dispatch collision events.
    auto agents = m_engine.getAllAgents();
    for (const auto& col : collisions) {
        // Push overlapping entities apart by the penetration depth,
        // weighted by inverse mass so lighter entities move more.
        double totalMass = col.massA + col.massB;
        if (totalMass > 1e-12) {
            double ratioA = col.massB / totalMass;
            double ratioB = col.massA / totalMass;

            // If one entity vastly outmasses the other, only push
            // the lighter one (a person can't shove a parked car).
            if (col.massA > col.massB * 10.0) {
                ratioA = 0.0; ratioB = 1.0;
            } else if (col.massB > col.massA * 10.0) {
                ratioA = 1.0; ratioB = 0.0;
            }
            for (auto& agent : agents) {
                if (agent->name() == col.nameA) {
                    auto loc = agent->location();
                    loc[0] -= col.normal[0] * col.penetration * ratioA;
                    loc[2] -= col.normal[2] * col.penetration * ratioA;
                    agent->set_location(loc);
                }
                if (agent->name() == col.nameB) {
                    auto loc = agent->location();
                    loc[0] += col.normal[0] * col.penetration * ratioB;
                    loc[2] += col.normal[2] * col.penetration * ratioB;
                    agent->set_location(loc);
                }
            }
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
