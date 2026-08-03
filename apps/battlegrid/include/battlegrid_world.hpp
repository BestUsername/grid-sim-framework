#ifndef BATTLEGRID_WORLD_HPP_INCLUDED
#define BATTLEGRID_WORLD_HPP_INCLUDED

#include "defines.hpp"
#include "terrain.hpp"
#include "soldier.hpp"
#include "civilian.hpp"
#include "vehicle.hpp"
#include "player_controller.hpp"
#include "input_map.hpp"

#include "libmap/map_world.hpp"
#include "libmap/ascii_format.hpp"
#include "libsim/base_engine.hpp"
#include "libsim/game_log.hpp"
#include "libphysics/physics_world.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace battlegrid {

/**
 * @brief The main game world that owns the terrain, engine, and entities.
 *
 * Loads a map, populates it with soldiers, civilians, and vehicles,
 * and provides the player controller for the player's soldier.
 */
class BattleGridWorld {
public:
    BattleGridWorld();

    /// Load the terrain map from a file (also populates the MapWorld).
    bool loadMap(const std::string& mapPath);

    /// Use an already-constructed terrain map (derives MapWorld from it).
    void loadMap(TerrainMap terrain);

    /// Populate the world with default entities.
    void populate(InputMap& inputMap);

    /// Access the simulation engine.
    BASE_ENGINE& engine() { return m_engine; }
    const BASE_ENGINE& engine() const { return m_engine; }

    /// Access the terrain map (used for physics / collision).
    const TerrainMap& terrainMap() const { return m_terrain; }

    /// Access the rich map world (used for display and export).
    const grid::libmap::MapWorld& mapWorld() const { return m_mapWorld; }

    /// Access the player controller.
    PlayerController& playerController() { return *m_playerController; }
    const PlayerController& playerController() const { return *m_playerController; }

    /// Access the player's soldier.
    Soldier& playerSoldier() { return *m_playerSoldier; }
    const Soldier& playerSoldier() const { return *m_playerSoldier; }

    /// Get all agents.
    auto getAllAgents() const { return m_engine.getAllAgents(); }

    /// Find the nearest vehicle to a position within range.
    Vehicle* findNearestVehicle(const COORD& pos, double range,
                                const std::unordered_map<std::string, COORD>& positions) const;

    /// Submit actor velocity intentions, step Box3D, apply solved transforms,
    /// and dispatch CollisionEvents to participating agents.
    void stepCollisions(double dt,
                        std::unordered_map<std::string, COORD>& positions);

    /// Transfer a soldier between its own dynamic body and a vehicle seat.
    bool mountSoldier(Soldier& soldier, Vehicle& vehicle);
    void dismountSoldier(Soldier& soldier, Vehicle& vehicle);

    /// Access the physics world (e.g. to change simulation mode).
    grid::physics::PhysicsWorld& physicsWorld() { return m_physicsWorld; }

private:
    void rebuildTerrainColliders();
    COORD findSpawnPoint(TerrainType required) const;
    void registerCollisionBody(const std::string& name, const COORD& pos,
                               double mass, double radius,
                               grid::physics::CollisionShape shape = grid::physics::CollisionShape::Sphere,
                               double capsuleHeight = 1.0,
                               grid::physics::Vec3 boxHalfExtents = {0.5, 0.5, 0.5},
                               double gravityScale = 1.0,
                               bool lockVerticalMotion = false, bool lockRotation = false,
                               bool lockYawRotation = true, double cylinderHeight = 0.3);
    void submitActorVelocities();
    void applySolvedTransforms(std::unordered_map<std::string, COORD>& positions);
    void retireDestroyedLandVehicles();
    void registerLandVehicleWheels(const LandVehicle& vehicle);
    void removeLandVehicleWheels(const LandVehicle& vehicle);

    BASE_ENGINE  m_engine;
    TerrainMap   m_terrain;
    grid::libmap::MapWorld m_mapWorld;
    grid::physics::PhysicsWorld m_physicsWorld;

    std::shared_ptr<Soldier> m_playerSoldier;
    std::unique_ptr<PlayerController> m_playerController;

    std::vector<std::shared_ptr<Soldier>>     m_soldiers;
    std::vector<std::shared_ptr<Civilian>>    m_civilians;
    std::vector<std::shared_ptr<LandVehicle>> m_landVehicles;
    std::vector<std::shared_ptr<SeaVehicle>>  m_seaVehicles;
    std::vector<std::shared_ptr<AirVehicle>>  m_airVehicles;
};

} // namespace battlegrid

#endif // BATTLEGRID_WORLD_HPP_INCLUDED
