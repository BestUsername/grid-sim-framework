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
#include "libnet/serializer.hpp"

#include <memory>
#include <functional>
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

    /// Dynamically add a soldier at runtime (e.g. for a remote player).
    std::shared_ptr<Soldier> addRemoteSoldier(const std::string& name);

    /// Create an agent from a network snapshot definition (for compute nodes).
    void addAgentFromSnapshot(const grid::net::AgentSnapshot& snap);

    /// Update local agents from server snapshots (client-side sync).
    /// Creates missing agents and updates yaw/health/dead on existing ones.
    void updateFromSnapshots(const std::vector<grid::net::AgentSnapshot>& snapshots);

    /// Mark an agent as remote-owned by name.  Returns true if found.
    bool setAgentRemoteOwned(const std::string& name, bool owned);
    /// Apply a server-authorized cross-node collision impulse to a local owner.
    bool applyCollisionCorrection(const std::string& name, const grid::physics::Vec3& impulse);
    void setCollisionCorrectionHandler(
        std::function<void(const std::string&, const grid::physics::Vec3&)> handler)
    {
        m_collisionCorrectionHandler = std::move(handler);
    }

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
                               bool lockYawRotation = true, double cylinderHeight = 0.3,
                               bool isBullet = false,
                               grid::physics::BodyMotion motion = grid::physics::BodyMotion::Dynamic);
    void submitActorVelocities();
    void applySolvedTransforms(std::unordered_map<std::string, COORD>& positions);
    void retireDestroyedLandVehicles();
    void registerLandVehicleWheels(const LandVehicle& vehicle);
    void removeLandVehicleWheels(const LandVehicle& vehicle);

    BASE_ENGINE  m_engine;
    TerrainMap   m_terrain;
    grid::libmap::MapWorld m_mapWorld;
    grid::physics::PhysicsWorld m_physicsWorld;
    std::function<void(const std::string&, const grid::physics::Vec3&)> m_collisionCorrectionHandler;
    std::unordered_map<std::string, grid::physics::Vec3> m_pendingCollisionImpulses;

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
