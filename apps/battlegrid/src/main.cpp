#include "defines.hpp"
#include "battlegrid_world.hpp"
#include "gl_display.hpp"
#include "input_map.hpp"
#include "player_controller.hpp"
#include "entity_types.hpp"
#include "terrain.hpp"
#include "sense_indicator.hpp"

#include "libsim/base_engine.hpp"
#include "libsim/game_log.hpp"
#include "libsim/types.hpp"
#include "libio/input_event.hpp"
#include "libio/keycodes.hpp"
#include "libevent/event.hpp"
#include "libnet/network_bridge.hpp"
#include "libnet/serializer.hpp"

#include <boost/system/system_error.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

using namespace grid::libsim;

enum class NetworkMode { Standalone, Server, Client, Headless, Compute };

// ── Server-side remote player state ─────────────────────────────────
struct RemotePlayer {
    std::shared_ptr<battlegrid::Soldier> soldier;
    grid::net::InputSnapshot input;
    double yaw   = 0.0;
    double pitch  = 0.3;
    battlegrid::Vehicle* vehicle = nullptr;
    bool prevInteract = false;
};

static void mergeRemoteInput(grid::net::InputSnapshot& pending,
                             const grid::net::InputSnapshot& incoming)
{
    pending.moveX = incoming.moveX;
    pending.moveZ = incoming.moveZ;
    pending.sprint = incoming.sprint;
    pending.lookDeltaX += incoming.lookDeltaX;
    pending.lookDeltaY += incoming.lookDeltaY;
    pending.zoomDelta += incoming.zoomDelta;
    pending.lookAxisX = incoming.lookAxisX;
    pending.lookAxisY = incoming.lookAxisY;
    pending.jump = pending.jump || incoming.jump;
    pending.interact = pending.interact || incoming.interact;
    pending.shout = pending.shout || incoming.shout;
    pending.toggleCamera = pending.toggleCamera || incoming.toggleCamera;
}

/// Apply an InputSnapshot to a remote soldier (mirrors PlayerController logic).
static void applyRemoteInput(RemotePlayer& rp, double dt,
                             battlegrid::BattleGridWorld& world,
                             const PositionSnapshot& positions)
{
    auto& soldier = *rp.soldier;
    auto& in = rp.input;
    const auto clearTransientInput = [&] {
        in.lookDeltaX = 0.0f;
        in.lookDeltaY = 0.0f;
        in.zoomDelta = 0.0f;
        in.jump = false;
        in.interact = false;
        in.shout = false;
        in.toggleCamera = false;
    };

    // Look
    rp.yaw   += static_cast<double>(in.lookDeltaX) * 0.003;
    rp.pitch  -= static_cast<double>(in.lookDeltaY) * 0.003;
    rp.yaw   += static_cast<double>(in.lookAxisX) * 3.0 * dt;
    rp.pitch  -= static_cast<double>(in.lookAxisY) * 3.0 * dt;
    rp.pitch   = std::clamp(rp.pitch, -1.4, 1.4);

    // Edge-triggered interact (mount / dismount)
    bool interactPressed = in.interact && !rp.prevInteract;
    rp.prevInteract = in.interact;
    if (interactPressed) {
        if (rp.vehicle) {
            world.dismountSoldier(soldier, *rp.vehicle);
            rp.vehicle = nullptr;
            soldier.setSpeedMultiplier(1.0);
        } else {
            COORD soldierPos = soldier.location();
            battlegrid::Vehicle* v = world.findNearestVehicle(soldierPos, 4.0, positions);
            if (v && !v->hasDriver()) {
                if (world.mountSoldier(soldier, *v)) {
                    rp.vehicle = v;
                }
            }
        }
    }

    // If mounted, move the vehicle instead
    if (rp.vehicle) {
        double moveX = static_cast<double>(in.moveX);
        double moveZ = static_cast<double>(in.moveZ);
        if (auto* landVehicle = dynamic_cast<battlegrid::LandVehicle*>(rp.vehicle)) {
            landVehicle->setDrivingControls(-moveZ, moveX);
        } else {
            const bool hasMove = std::abs(moveX) > 0.01 || std::abs(moveZ) > 0.01;
            if (!hasMove) {
                rp.vehicle->clearMoveTarget();
                clearTransientInput();
                return;
            }
            const double length = std::hypot(moveX, moveZ);
            if (length > 1.0) {
                moveX /= length;
                moveZ /= length;
            }
            const double worldX = -moveZ * std::cos(rp.yaw) - moveX * std::sin(rp.yaw);
            const double worldZ = -moveZ * std::sin(rp.yaw) + moveX * std::cos(rp.yaw);
            rp.vehicle->setYaw(std::atan2(worldZ, worldX));
            rp.vehicle->setMoveTarget(COORD{
                rp.vehicle->location()[0] + worldX * rp.vehicle->speed(),
                rp.vehicle->location()[1],
                rp.vehicle->location()[2] + worldZ * rp.vehicle->speed()});
        }
        clearTransientInput();
        return;
    }

    // Movement
    double moveX = static_cast<double>(in.moveX);
    double moveZ = static_cast<double>(in.moveZ);
    bool hasMove = std::abs(moveX) > 0.01 || std::abs(moveZ) > 0.01;

    if (hasMove) {
        double len = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (len > 1.0) { moveX /= len; moveZ /= len; }
        double cosY = std::cos(rp.yaw);
        double sinY = std::sin(rp.yaw);
        double worldX = -moveZ * cosY - moveX * sinY;
        double worldZ = -moveZ * sinY + moveX * cosY;
        soldier.setYaw(std::atan2(worldZ, worldX));

        double sprintFactor = std::clamp(static_cast<double>(in.sprint), 0.0, 1.0);
        double speedMul = 1.0 + sprintFactor;
        soldier.setSpeedMultiplier(speedMul);

        soldier.setMovementVelocity(
            worldX * soldier.speed() * speedMul,
            worldZ * soldier.speed() * speedMul);
    } else {
        soldier.setMovementVelocity(0.0, 0.0);
    }

    if (in.jump)
        soldier.jump();
    if (in.shout)
        soldier.communicate(grid::libsim::Senses::Hearing, "Hey! Over here!");

    clearTransientInput();
}

/// Snapshot every agent into a vector of AgentSnapshots for network broadcast.
static std::vector<grid::net::AgentSnapshot> snapshotAllAgents(
    const std::vector<std::shared_ptr<I_AGENT>>& agents,
    const PositionSnapshot& positions)
{
    std::vector<grid::net::AgentSnapshot> out;
    out.reserve(agents.size());
    for (auto& a : agents) {
        grid::net::AgentSnapshot snap;
        snap.name = a->name();
        auto it = positions.find(a->name());
        if (it != positions.end()) {
            snap.position = {it->second[0], it->second[1], it->second[2]};
        }
        // Downcast to extract battlegrid-specific fields
        if (auto* s = dynamic_cast<battlegrid::Soldier*>(a.get())) {
            snap.health     = s->health();
            snap.yaw        = s->yaw();
            snap.entityType = static_cast<uint8_t>(s->entityType());
            snap.faction    = static_cast<uint8_t>(s->faction());
            snap.dead       = s->isDead();
        } else if (auto* v = dynamic_cast<battlegrid::Vehicle*>(a.get())) {
            snap.health     = v->health();
            snap.yaw        = v->yaw();
            snap.entityType = static_cast<uint8_t>(v->entityType());
            snap.faction    = static_cast<uint8_t>(v->faction());
            snap.dead       = v->isDead();
            if (v->hasDriver())
                snap.driverName = v->driver()->name();
        } else if (auto* c = dynamic_cast<battlegrid::Civilian*>(a.get())) {
            snap.health     = c->health();
            snap.entityType = static_cast<uint8_t>(battlegrid::EntityType::Civilian);
            snap.dead       = c->isDead();
        }
        out.push_back(std::move(snap));
    }
    return out;
}

/// Capture the current InputMap state into an InputSnapshot for the server.
static grid::net::InputSnapshot buildInputSnapshot(battlegrid::InputMap& inputMap)
{
    grid::net::InputSnapshot snap;
    snap.moveX      = inputMap.axis(battlegrid::GameAction::MoveX);
    snap.moveZ      = inputMap.axis(battlegrid::GameAction::MoveZ);
    snap.sprint     = inputMap.axis(battlegrid::GameAction::Sprint);
    snap.lookDeltaX = inputMap.delta(battlegrid::GameAction::LookX);
    snap.lookDeltaY = inputMap.delta(battlegrid::GameAction::LookY);
    snap.zoomDelta  = inputMap.delta(battlegrid::GameAction::Zoom);
    snap.lookAxisX  = inputMap.axis(battlegrid::GameAction::LookX);
    snap.lookAxisY  = inputMap.axis(battlegrid::GameAction::LookY);
    snap.jump         = inputMap.pressed(battlegrid::GameAction::Jump);
    snap.interact     = inputMap.pressed(battlegrid::GameAction::Interact);
    snap.shout        = inputMap.pressed(battlegrid::GameAction::Shout);
    snap.toggleCamera = inputMap.pressed(battlegrid::GameAction::ToggleCamera);
    return snap;
}

/// Serialize a TerrainMap into a TerrainData message.
/// Wire format: [uint16 width] [uint16 height] [width*height chars]
static grid::net::Message serializeTerrain(const battlegrid::TerrainMap& map) {
    if (map.width() > std::numeric_limits<uint16_t>::max()
        || map.height() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("terrain dimensions exceed network protocol limit");
    }
    std::vector<uint8_t> buf;
    uint16_t w = static_cast<uint16_t>(map.width());
    uint16_t h = static_cast<uint16_t>(map.height());
    buf.resize(4 + static_cast<size_t>(w) * h);
    std::memcpy(buf.data(), &w, 2);
    std::memcpy(buf.data() + 2, &h, 2);
    size_t off = 4;
    for (size_t z = 0; z < h; ++z)
        for (size_t x = 0; x < w; ++x)
            buf[off++] = static_cast<uint8_t>(map.at(x, z));
    return grid::net::Message(grid::net::MessageType::TerrainData, std::move(buf));
}

/// Deserialize a TerrainData message into a TerrainMap.
static battlegrid::TerrainMap deserializeTerrain(const grid::net::Message& msg) {
    grid::net::detail::Reader reader(msg.payload());
    const uint16_t w = reader.readUint16();
    const uint16_t h = reader.readUint16();
    if (reader.remaining() != static_cast<size_t>(w) * h) {
        throw std::invalid_argument("invalid terrain payload size");
    }
    battlegrid::TerrainMap map(w, h, battlegrid::TerrainType::Land);
    for (size_t z = 0; z < h; ++z)
        for (size_t x = 0; x < w; ++x) {
            const auto terrain = static_cast<battlegrid::TerrainType>(reader.readUint8());
            switch (terrain) {
            case battlegrid::TerrainType::Water:
            case battlegrid::TerrainType::Land:
            case battlegrid::TerrainType::Bump:
            case battlegrid::TerrainType::SlopeNorth:
            case battlegrid::TerrainType::SlopeSouth:
            case battlegrid::TerrainType::SlopeEast:
            case battlegrid::TerrainType::SlopeWest:
            case battlegrid::TerrainType::Hill:
            case battlegrid::TerrainType::Mountain:
                map.set(x, z, terrain);
                break;
            default:
                throw std::invalid_argument("invalid terrain value");
            }
        }
    return map;
}

/// Serialize a batch of game log entries.
/// Wire: [uint16 count] per-entry: [string source][string location][uint8 sense][string message]
static grid::net::Message serializeGameLogBatch(
    const std::vector<grid::libsim::LogEntry>& entries,
    size_t from, size_t to)
{
    if (to < from || to - from > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("game log batch exceeds network protocol limit");
    }
    std::vector<uint8_t> buf;
    uint16_t count = static_cast<uint16_t>(to - from);
    buf.resize(2);
    std::memcpy(buf.data(), &count, 2);
    for (size_t i = from; i < to; ++i) {
        grid::net::detail::packString(buf, entries[i].source);
        grid::net::detail::packString(buf, entries[i].location);
        grid::net::detail::packUint8(buf, static_cast<uint8_t>(entries[i].sense));
        grid::net::detail::packString(buf, entries[i].message);
    }
    return grid::net::Message(grid::net::MessageType::GameLogBatch, std::move(buf));
}

/// Deserialize a batch of game log entries.
struct RemoteLogEntry {
    std::string source;
    std::string location;
    grid::libsim::Senses sense;
    std::string message;
};

static std::vector<RemoteLogEntry> deserializeGameLogBatch(
    const grid::net::Message& msg)
{
    grid::net::detail::Reader reader(msg.payload());
    const uint16_t count = reader.readUint16();
    std::vector<RemoteLogEntry> out(count);
    for (uint16_t i = 0; i < count; ++i) {
        out[i].source = reader.readString();
        out[i].location = reader.readString();
        const auto sense = reader.readUint8();
        if (sense > static_cast<uint8_t>(grid::libsim::Senses::Touch)) {
            throw std::invalid_argument("invalid game log sense");
        }
        out[i].sense = static_cast<grid::libsim::Senses>(sense);
        out[i].message = reader.readString();
    }
    if (!reader.empty()) {
        throw std::invalid_argument("game log payload contains trailing data");
    }
    return out;
}

int main(int argc, char** argv)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    // ── Parse arguments ─────────────────────────────────────────────
    std::string mapPath;
    NetworkMode networkMode = NetworkMode::Standalone;
    uint16_t serverPort = 0;
    std::string clientHost;
    uint16_t clientPort = 0;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--map") == 0 && i + 1 < argc) {
            mapPath = argv[++i];
        } else if (std::strcmp(argv[i], "--server") == 0 && i + 1 < argc) {
            networkMode = NetworkMode::Server;
            serverPort = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else if (std::strcmp(argv[i], "--headless") == 0 && i + 1 < argc) {
            networkMode = NetworkMode::Headless;
            serverPort = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else if (std::strcmp(argv[i], "--compute") == 0 && i + 1 < argc) {
            networkMode = NetworkMode::Compute;
            std::string hostPort = argv[++i];
            auto colon = hostPort.rfind(':');
            if (colon == std::string::npos) {
                std::cerr << "Invalid --compute format. Use --compute host:port\n";
                return EXIT_FAILURE;
            }
            clientHost = hostPort.substr(0, colon);
            clientPort = static_cast<uint16_t>(std::atoi(hostPort.substr(colon + 1).c_str()));
        } else if (std::strcmp(argv[i], "--client") == 0 && i + 1 < argc) {
            networkMode = NetworkMode::Client;
            std::string hostPort = argv[++i];
            auto colon = hostPort.rfind(':');
            if (colon == std::string::npos) {
                std::cerr << "Invalid --client format. Use --client host:port\n";
                return EXIT_FAILURE;
            }
            clientHost = hostPort.substr(0, colon);
            clientPort = static_cast<uint16_t>(std::atoi(hostPort.substr(colon + 1).c_str()));
        }
    }

    // ── Create world ────────────────────────────────────────────────
    battlegrid::BattleGridWorld world;

    if (networkMode == NetworkMode::Client || networkMode == NetworkMode::Compute) {
        // Client/Compute: load a placeholder terrain; real terrain arrives from server.
        battlegrid::TerrainMap placeholder(16, 16, battlegrid::TerrainType::Land);
        world.loadMap(std::move(placeholder));
    } else if (!mapPath.empty()) {
        if (!world.loadMap(mapPath)) {
            return EXIT_FAILURE;
        }
    } else {
        // Generate a default map inline
        std::cout << "No map specified (use --map <path>). Using built-in default map.\n";
        battlegrid::TerrainMap defaultMap(64, 64, battlegrid::TerrainType::Land);

        for (size_t z = 0; z < 64; ++z) {
            for (size_t x = 0; x < 64; ++x) {
                // River / water along left and top edges
                if (x < 8 || z < 5) {
                    defaultMap.set(x, z, battlegrid::TerrainType::Water);
                }
                // Mountain range in the bottom-right
                if (x > 45 && z > 40 && (x + z) > 95) {
                    defaultMap.set(x, z, battlegrid::TerrainType::Mountain);
                }
                // Small lake in center
                double cx = static_cast<double>(x) - 32.0;
                double cz = static_cast<double>(z) - 25.0;
                if (cx * cx + cz * cz < 36.0) {
                    defaultMap.set(x, z, battlegrid::TerrainType::Water);
                }
            }
        }
        // Keep the no-argument demo map comparable to maps/default.map: a
        // half-metre plateau approached from each side by physical ramp tiles.
        defaultMap.set(20, 19, battlegrid::TerrainType::SlopeSouth);
        defaultMap.set(19, 20, battlegrid::TerrainType::SlopeEast);
        defaultMap.set(20, 20, battlegrid::TerrainType::Hill);
        defaultMap.set(21, 20, battlegrid::TerrainType::SlopeWest);
        defaultMap.set(20, 21, battlegrid::TerrainType::SlopeNorth);

        world.loadMap(std::move(defaultMap));
    }

    // ── Configure input mapping ──────────────────────────────────
    battlegrid::InputMap inputMap;

    // Keyboard: movement
    inputMap.bindKey(io::Key::W,          battlegrid::GameAction::MoveZ,  -1.0f);
    inputMap.bindKey(io::Key::S,          battlegrid::GameAction::MoveZ,   1.0f);
    inputMap.bindKey(io::Key::A,          battlegrid::GameAction::MoveX,  -1.0f);
    inputMap.bindKey(io::Key::D,          battlegrid::GameAction::MoveX,   1.0f);
    inputMap.bindKey(io::Key::LeftShift,  battlegrid::GameAction::Sprint,  1.0f);
    inputMap.bindKey(io::Key::RightShift, battlegrid::GameAction::Sprint,  1.0f);

    // Keyboard: actions
    inputMap.bindKey(io::Key::Space, battlegrid::GameAction::Jump);
    inputMap.bindKey(io::Key::E,   battlegrid::GameAction::Interact);
    inputMap.bindKey(io::Key::T,   battlegrid::GameAction::Shout);
    inputMap.bindKey(io::Key::Tab, battlegrid::GameAction::ToggleCamera);
    inputMap.bindKey(io::Key::M,   battlegrid::GameAction::ToggleMap);

    // Mouse: look + zoom
    inputMap.bindMouseX(battlegrid::GameAction::LookX, 1.0f);
    inputMap.bindMouseY(battlegrid::GameAction::LookY, 1.0f);
    inputMap.bindScrollY(battlegrid::GameAction::Zoom, 0.5f);

    // Gamepad: movement (left stick)
    inputMap.bindAxis(io::GamepadAxis::LeftX, battlegrid::GameAction::MoveX, 1.0f);
    inputMap.bindAxis(io::GamepadAxis::LeftY, battlegrid::GameAction::MoveZ, 1.0f);

    // Gamepad: look (right stick)
    inputMap.bindAxis(io::GamepadAxis::RightX, battlegrid::GameAction::LookX, 1.0f);
    inputMap.bindAxis(io::GamepadAxis::RightY, battlegrid::GameAction::LookY, 1.0f);

    // Gamepad: sprint (left trigger, analog)
    inputMap.bindAxis(io::GamepadAxis::LeftTrigger, battlegrid::GameAction::Sprint, 1.0f);

    // Gamepad: actions
    inputMap.bindButton(io::GamepadButton::B,          battlegrid::GameAction::Jump);
    inputMap.bindButton(io::GamepadButton::A,          battlegrid::GameAction::Interact);
    inputMap.bindButton(io::GamepadButton::Y,          battlegrid::GameAction::Shout);
    inputMap.bindButton(io::GamepadButton::LeftBumper,  battlegrid::GameAction::ToggleCamera);

    // Gamepad: digital movement (D-pad)
    inputMap.bindButton(io::GamepadButton::DPadUp,    battlegrid::GameAction::MoveZ, -1.0f);
    inputMap.bindButton(io::GamepadButton::DPadDown,  battlegrid::GameAction::MoveZ,  1.0f);
    inputMap.bindButton(io::GamepadButton::DPadLeft,  battlegrid::GameAction::MoveX, -1.0f);
    inputMap.bindButton(io::GamepadButton::DPadRight, battlegrid::GameAction::MoveX,  1.0f);

    // Compute nodes don't need a local player — agents arrive from server.
    if (networkMode != NetworkMode::Compute)
        world.populate(inputMap);

    const bool hasDisplay = (networkMode != NetworkMode::Headless
                          && networkMode != NetworkMode::Compute);
    const bool isServerLike = (networkMode == NetworkMode::Server
                            || networkMode == NetworkMode::Headless);

    // ── Announce network mode ───────────────────────────────────────
    switch (networkMode) {
    case NetworkMode::Server:
        std::cout << "Starting in SERVER mode on port " << serverPort << "\n";
        break;
    case NetworkMode::Headless:
        std::cout << "Starting in HEADLESS server mode on port " << serverPort << "\n";
        break;
    case NetworkMode::Client:
        std::cout << "Starting in CLIENT mode, connecting to "
                  << clientHost << ":" << clientPort << "\n";
        break;
    case NetworkMode::Compute:
        std::cout << "Starting in COMPUTE node mode, connecting to "
                  << clientHost << ":" << clientPort << "\n";
        break;
    default:
        std::cout << "Starting in standalone mode.\n";
        break;
    }

    // ── Network setup (server / headless mode) ────────────────────
    grid::net::NetworkBridge bridge;
    std::mutex remotesMtx;
    std::unordered_map<grid::net::Session*, RemotePlayer> remotePlayers;
    // Pending connections handled on the game-loop thread
    std::mutex pendingMtx;
    std::vector<std::shared_ptr<grid::net::Session>> pendingConnects;
    std::vector<grid::net::Session*> pendingDisconnects;
    std::atomic<int> nextClientId{1};

    // Compute-node tracking (server side)
    struct ComputeNode {
        std::shared_ptr<grid::net::Session> session;
        std::vector<std::string> ownedAgents;
    };
    std::mutex computeMtx;
    std::unordered_map<grid::net::Session*, ComputeNode> computeNodes;
    std::vector<std::shared_ptr<grid::net::Session>> pendingComputeNodes;
    // Latest position data received from compute nodes
    std::mutex computePosMtx;
    std::unordered_map<std::string, grid::net::AgentSnapshot> computeSnapshots;

    if (isServerLike) {
        try {
        bridge.hostServer(
            serverPort,
            // Per-session message handler (runs on IO thread)
            [&](std::shared_ptr<grid::net::Session> session, grid::net::Message msg) {
                if (msg.type() == grid::net::MessageType::InputEvent &&
                    msg.payload().size() == grid::net::InputSnapshot::kSerializedSize) {
                    auto input = grid::net::deserializeInputSnapshot(msg);
                    std::lock_guard<std::mutex> lk(remotesMtx);
                    auto it = remotePlayers.find(session.get());
                    if (it != remotePlayers.end())
                        mergeRemoteInput(it->second.input, input);
                } else if (msg.type() == grid::net::MessageType::ComputeRegister) {
                    std::lock_guard<std::mutex> lk(pendingMtx);
                    pendingComputeNodes.push_back(session);
                } else if (msg.type() == grid::net::MessageType::AgentState) {
                    // Solved state updates from a compute node.
                    auto snaps = grid::net::deserializeAgentStates(msg);
                    std::lock_guard<std::mutex> lk(computePosMtx);
                    for (auto& s : snaps) {
                        computeSnapshots[s.name] = std::move(s);
                    }
                }
            },
            // Connect handler
            [&](std::shared_ptr<grid::net::Session> session) {
                std::lock_guard<std::mutex> lk(pendingMtx);
                pendingConnects.push_back(session);
            },
            // Disconnect handler
            [&](std::shared_ptr<grid::net::Session> session) {
                std::lock_guard<std::mutex> lk(pendingMtx);
                pendingDisconnects.push_back(session.get());
            });
        } catch (const boost::system::system_error& error) {
            std::cerr << "Unable to listen on port " << serverPort << ": " << error.what() << "\n";
            return EXIT_FAILURE;
        }
    }

    // ── Network setup (client mode) ─────────────────────────────────
    std::mutex clientSnapshotMtx;
    std::vector<grid::net::AgentSnapshot> clientSnapshots;
    std::string myPlayerName;  // assigned by server
    std::atomic<bool> playerAssigned{false};
    std::mutex terrainMtx;
    std::unique_ptr<battlegrid::TerrainMap> receivedTerrain;
    std::atomic<bool> terrainReceived{false};
    std::mutex clientLogMtx;
    std::vector<RemoteLogEntry> pendingLogEntries;

    // Compute-node client state
    std::mutex assignmentMtx;
    std::vector<grid::net::AgentSnapshot> pendingAssignments;
    std::atomic<bool> assignmentsReceived{false};
    std::mutex correctionMtx;
    std::vector<grid::net::CollisionCorrection> pendingCorrections;

    if (networkMode == NetworkMode::Client) {
        bridge.connectToServer(clientHost, clientPort,
            [&](grid::net::Message msg) {
                if (msg.type() == grid::net::MessageType::AgentState) {
                    auto snaps = grid::net::deserializeAgentStates(msg);
                    std::lock_guard<std::mutex> lk(clientSnapshotMtx);
                    clientSnapshots = std::move(snaps);
                } else if (msg.type() == grid::net::MessageType::BusEvent) {
                    myPlayerName = grid::net::deserializePlayerAssignment(msg);
                    playerAssigned = true;
                    std::cout << "Assigned soldier: " << myPlayerName << "\n";
                } else if (msg.type() == grid::net::MessageType::TerrainData) {
                    std::lock_guard<std::mutex> lk(terrainMtx);
                    receivedTerrain = std::make_unique<battlegrid::TerrainMap>(
                        deserializeTerrain(msg));
                    terrainReceived = true;
                    std::cout << "Received terrain from server.\n";
                } else if (msg.type() == grid::net::MessageType::GameLogBatch) {
                    auto entries = deserializeGameLogBatch(msg);
                    std::lock_guard<std::mutex> lk(clientLogMtx);
                    pendingLogEntries.insert(pendingLogEntries.end(),
                        std::make_move_iterator(entries.begin()),
                        std::make_move_iterator(entries.end()));
                }
            });

        // Wait for terrain data before creating the display
        std::cout << "Waiting for server terrain data...\n";
        while (!terrainReceived) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        {
            std::lock_guard<std::mutex> lk(terrainMtx);
            world.loadMap(std::move(*receivedTerrain));
        }
    }

    // ── Network setup (compute node mode) ───────────────────────────
    if (networkMode == NetworkMode::Compute) {
        bridge.connectToServer(clientHost, clientPort,
            [&](grid::net::Message msg) {
                if (msg.type() == grid::net::MessageType::TerrainData) {
                    std::lock_guard<std::mutex> lk(terrainMtx);
                    receivedTerrain = std::make_unique<battlegrid::TerrainMap>(
                        deserializeTerrain(msg));
                    terrainReceived = true;
                    std::cout << "[Compute] Received terrain from server.\n";
                } else if (msg.type() == grid::net::MessageType::AgentAssignment) {
                    auto snaps = grid::net::deserializeAgentAssignment(msg);
                    std::lock_guard<std::mutex> lk(assignmentMtx);
                    pendingAssignments = std::move(snaps);
                    assignmentsReceived = true;
                    std::cout << "[Compute] Received " << pendingAssignments.size()
                              << " agent assignments.\n";
                } else if (msg.type() == grid::net::MessageType::CollisionCorrection) {
                    auto correction = grid::net::deserializeCollisionCorrection(msg);
                    std::lock_guard<std::mutex> lk(correctionMtx);
                    pendingCorrections.push_back(std::move(correction));
                }
            });

        // Identify ourselves as a compute node
        bridge.sendToServer(grid::net::serializeComputeRegister());

        // Wait for terrain and assignments
        std::cout << "[Compute] Waiting for server data...\n";
        while (!terrainReceived || !assignmentsReceived) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        {
            std::lock_guard<std::mutex> lk(terrainMtx);
            world.loadMap(std::move(*receivedTerrain));
        }
        {
            std::lock_guard<std::mutex> lk(assignmentMtx);
            for (auto& snap : pendingAssignments)
                world.addAgentFromSnapshot(snap);
        }
    }

    // ── Create 3D display (skip for headless / compute) ─────────────
    std::unique_ptr<battlegrid::GLDisplay> display;
    if (hasDisplay)
        display = std::make_unique<battlegrid::GLDisplay>(
            world.terrainMap(), world.mapWorld(), 1280, 720);

    // ── Run the simulation engine on a separate thread ──────────────
    std::thread engineThread = world.engine().run();
    if (networkMode == NetworkMode::Client)
        world.engine().setState(State::PAUSED);

    // PlayerController is only available when populate() was called.
    auto* playerCtrlPtr = (networkMode != NetworkMode::Compute)
        ? &world.playerController() : nullptr;
    auto lastFrame = std::chrono::steady_clock::now();
    size_t lastLogIndex = 0; // track how many log entries we've printed
    size_t serverLogSentIndex = 0; // server: track how many log entries sent to clients
    battlegrid::SenseIndicatorManager senseIndicators;
    if (isServerLike) {
        world.setCollisionCorrectionHandler(
            [&](const std::string& targetName, const grid::physics::Vec3& impulse) {
                std::lock_guard<std::mutex> lk(computeMtx);
                for (const auto& [session, node] : computeNodes) {
                    if (std::find(node.ownedAgents.begin(), node.ownedAgents.end(), targetName)
                        != node.ownedAgents.end()) {
                        node.session->send(grid::net::serializeCollisionCorrection({targetName, impulse}));
                        return;
                    }
                }
            });
    }

    // ── SIGINT handler for headless / compute graceful shutdown ──────
    static std::atomic<bool> g_quit{false};
    std::signal(SIGINT, [](int) { g_quit.store(true); });

    // ── Real-time game loop ─────────────────────────────────────────
    bool running  = true;
    bool showMap  = false;
    while (running && !g_quit.load()) {
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - lastFrame).count();
        lastFrame = now;

        // Take a thread-safe position snapshot for this frame
        auto positions = world.engine().snapshotAgentPositions();

        // ── Server / Headless: handle pending connect / disconnect ───
        if (isServerLike) {
            world.engine().withAgentsLock([&] {
            std::lock_guard<std::mutex> lk(pendingMtx);

            // Handle compute node registrations
            for (auto& s : pendingComputeNodes) {
                ComputeNode cn;
                cn.session = s;

                // Send terrain
                s->send(serializeTerrain(world.terrainMap()));

                // Build assignment: delegate all non-player NPCs
                std::vector<grid::net::AgentSnapshot> assignments;
                auto agents = world.engine().getAllAgents();
                for (auto& a : agents) {
                    // Vehicles remain server-authoritative so clients can mount
                    // and drive them without crossing a physics-owner boundary.
                    if (a->name() == "Player") continue;
                    if (dynamic_cast<battlegrid::Vehicle*>(a.get())) continue;
                    {
                        std::lock_guard<std::mutex> rlk(remotesMtx);
                        bool isClient = false;
                        for (auto& [sess, rp] : remotePlayers) {
                            if (rp.soldier->name() == a->name()) { isClient = true; break; }
                        }
                        if (isClient) continue;
                    }
                    // Skip agents already assigned to another compute node
                    {
                        std::lock_guard<std::mutex> clk(computeMtx);
                        bool alreadyOwned = false;
                        for (auto& [sess, existing] : computeNodes) {
                            for (auto& n : existing.ownedAgents) {
                                if (n == a->name()) { alreadyOwned = true; break; }
                            }
                            if (alreadyOwned) break;
                        }
                        if (alreadyOwned) continue;
                    }

                    // Build snapshot for this agent
                    grid::net::AgentSnapshot snap;
                    snap.name = a->name();
                    auto pit = positions.find(a->name());
                    if (pit != positions.end())
                        snap.position = {pit->second[0], pit->second[1], pit->second[2]};
                    if (auto* sol = dynamic_cast<battlegrid::Soldier*>(a.get())) {
                        snap.health     = sol->health();
                        snap.yaw        = sol->yaw();
                        snap.entityType = static_cast<uint8_t>(sol->entityType());
                        snap.faction    = static_cast<uint8_t>(sol->faction());
                        snap.dead       = sol->isDead();
                    } else if (auto* civ = dynamic_cast<battlegrid::Civilian*>(a.get())) {
                        snap.health     = civ->health();
                        snap.entityType = static_cast<uint8_t>(battlegrid::EntityType::Civilian);
                        snap.dead       = civ->isDead();
                    } else if (auto* veh = dynamic_cast<battlegrid::Vehicle*>(a.get())) {
                        snap.health     = veh->health();
                        snap.yaw        = veh->yaw();
                        snap.entityType = static_cast<uint8_t>(veh->entityType());
                        snap.faction    = static_cast<uint8_t>(veh->faction());
                        snap.dead       = veh->isDead();
                    }
                    assignments.push_back(snap);
                    cn.ownedAgents.push_back(a->name());
                }

                // Mark assigned agents as remote-owned on the server
                for (auto& name : cn.ownedAgents)
                    world.setAgentRemoteOwned(name, true);

                s->send(grid::net::serializeAgentAssignment(assignments));
                std::cout << "Compute node connected, delegated "
                          << cn.ownedAgents.size() << " agents.\n";
                {
                    std::lock_guard<std::mutex> clk(computeMtx);
                    computeNodes[s.get()] = std::move(cn);
                }
            }
            pendingComputeNodes.clear();

            // Handle regular client connections
            for (auto& s : pendingConnects) {
                // Skip if this is a compute node (already handled above)
                {
                    std::lock_guard<std::mutex> clk(computeMtx);
                    if (computeNodes.count(s.get())) continue;
                }
                std::string name = "Client_" + std::to_string(nextClientId++);
                auto soldier = world.addRemoteSoldier(name);
                s->send(grid::net::serializePlayerAssignment(name));
                s->send(serializeTerrain(world.terrainMap()));
                std::lock_guard<std::mutex> rlk(remotesMtx);
                remotePlayers[s.get()] = {soldier, {}, 0.0, 0.3};
            }
            pendingConnects.clear();

            // Handle disconnections
            for (auto* s : pendingDisconnects) {
                {
                    std::lock_guard<std::mutex> rlk(remotesMtx);
                    remotePlayers.erase(s);
                }
                // If it was a compute node, unfreeze its agents
                {
                    std::lock_guard<std::mutex> clk(computeMtx);
                    auto it = computeNodes.find(s);
                    if (it != computeNodes.end()) {
                        for (auto& name : it->second.ownedAgents)
                            world.setAgentRemoteOwned(name, false);
                        std::cout << "Compute node disconnected, reclaimed "
                                  << it->second.ownedAgents.size() << " agents.\n";
                        computeNodes.erase(it);
                    }
                }
            }
            pendingDisconnects.clear();
            });
        }

        // Drain input events from SDL (only when we have a display)
        if (display) {
            if (!display->hasInputFocus()) {
                inputMap.clear();
            }
            while (auto evt = display->pollEvent()) {
                // Check for quit
                if (auto* ke = std::get_if<io::KeyEvent>(&*evt)) {
                    if (ke->key == io::Key::Q && ke->action == io::Action::Press) {
                        running = false;
                        break;
                    }
                    if (ke->key == io::Key::P && ke->action == io::Action::Press) {
                        world.engine().setState(
                            world.engine().getState() == State::RUNNING
                            ? State::PAUSED : State::RUNNING);
                        continue;
                    }
                    if (ke->key == io::Key::Escape && ke->action == io::Action::Press) {
                        running = false;
                        break;
                    }
                }

                // Feed all input through the configurable input map
                inputMap.processEvent(*evt);
            }
            if (!display->hasInputFocus()) {
                inputMap.clear();
            }

            // Handle map toggle
            if (inputMap.pressed(battlegrid::GameAction::ToggleMap))
                showMap = !showMap;
        }

        // ── Client: send local input to server ─────────────────────
        if (networkMode == NetworkMode::Client) {
            auto snap = buildInputSnapshot(inputMap);
            bridge.sendToServer(grid::net::serializeInputSnapshot(snap));

            // Apply server snapshots to the position map and sync
            // agent state (yaw, health, etc.).  Creates any agents
            // that don't exist locally yet (e.g. our assigned soldier
            // or other connected clients).
            {
                std::lock_guard<std::mutex> lk(clientSnapshotMtx);
                world.engine().withAgentsLock([&] {
                    world.updateFromSnapshots(clientSnapshots);
                    for (auto& s : clientSnapshots) {
                        positions[s.name] = COORD{s.position[0], s.position[1], s.position[2]};
                    }
                });
            }

            // Update the local camera to follow our assigned soldier
            if (playerAssigned) {
                // Mark our soldier as player-controlled once for HUD
                static bool playerMarked = false;
                if (!playerMarked) {
                    for (auto& a : world.engine().getAllAgents()) {
                        if (a->name() == myPlayerName) {
                            if (auto* s = dynamic_cast<battlegrid::Soldier*>(a.get()))
                                s->setPlayerControlled(true);
                            playerMarked = true;
                            break;
                        }
                    }
                }

                auto it = positions.find(myPlayerName);
                if (it != positions.end())
                    playerCtrlPtr->setSnapshotPosition(it->second);
            }

            // Process camera look / zoom / mode toggle locally so the
            // player can rotate the view even though the server owns
            // movement.  This keeps client-side yaw in sync with the
            // server (both consume the same deltas).
            playerCtrlPtr->updateCamera(dt);
        }

        // ── Compute node: send position updates to server ──────────
        if (networkMode == NetworkMode::Compute) {
            auto agents = world.engine().getAllAgents();
            auto snapshots = snapshotAllAgents(agents, positions);
            bridge.sendToServer(grid::net::serializeAgentStates(snapshots));
        }

        // Update player movement and check collisions, both under the
        // agents lock to serialise with the engine thread.
        // (In client mode, the server owns the simulation — skip local physics.)
        // (In compute mode, the engine handles AI; we just need collisions.)
        if (networkMode != NetworkMode::Client && networkMode != NetworkMode::Compute) {
            world.engine().withAgentsLock([&] {
                playerCtrlPtr->update(dt, positions);

                // Refresh the player (and vehicle) position in the snapshot
                // so collision detection and rendering see the latest location.
                positions["Player"] = world.playerSoldier().location();
                if (playerCtrlPtr->inVehicle())
                    positions[playerCtrlPtr->currentVehicle()->name()] =
                        playerCtrlPtr->currentVehicle()->location();

                // Set the camera snapshot under the lock so it matches the
                // positions the renderer will use (no frame-lag from engine ticks
                // that ran between the snapshot and this lock acquisition).
                if (hasDisplay) {
                    std::string entityName = playerCtrlPtr->inVehicle()
                        ? playerCtrlPtr->currentVehicle()->name() : std::string("Player");
                    auto it = positions.find(entityName);
                    if (it != positions.end())
                        playerCtrlPtr->setSnapshotPosition(it->second);
                }

                // ── Server / Headless: apply remote client inputs ───────
                if (isServerLike) {
                    std::lock_guard<std::mutex> rlk(remotesMtx);
                    for (auto& [sess, rp] : remotePlayers) {
                        applyRemoteInput(rp, dt, world, positions);
                        positions[rp.soldier->name()] = rp.soldier->location();
                        if (rp.vehicle)
                            positions[rp.vehicle->name()] = rp.vehicle->location();
                    }
                }

                // ── Server / Headless: apply compute node positions ─────
                if (isServerLike) {
                    std::lock_guard<std::mutex> lk(computePosMtx);
                    std::vector<grid::net::AgentSnapshot> snapshots;
                    snapshots.reserve(computeSnapshots.size());
                    for (const auto& [name, snapshot] : computeSnapshots) {
                        snapshots.push_back(snapshot);
                        positions[name] = COORD{
                            snapshot.position[0], snapshot.position[1], snapshot.position[2]};
                    }
                    world.updateFromSnapshots(snapshots);
                }

                world.stepCollisions(dt, positions);
            });
        }

        // Compute nodes run their own collision step
        if (networkMode == NetworkMode::Compute) {
            world.engine().withAgentsLock([&] {
                std::lock_guard<std::mutex> lk(correctionMtx);
                for (const auto& correction : pendingCorrections) {
                    world.applyCollisionCorrection(correction.targetName, correction.impulse);
                }
                pendingCorrections.clear();
                world.stepCollisions(dt, positions);
            });
        }

        // Update sense indicators
        senseIndicators.update(static_cast<float>(dt));

        // Render the 3D scene (only with a display)
        auto agents = world.engine().getAllAgents();
        if (display) {
            display->renderFrame(agents,
                                positions,
                                world.engine().getState(),
                                world.engine().getGameLog(),
                                *playerCtrlPtr,
                                senseIndicators,
                                showMap);
        }

        // ── Server / Headless: broadcast agent states to clients ────
        if (isServerLike) {
            auto snapshots = snapshotAllAgents(agents, positions);
            bridge.broadcastAgentStates(snapshots);

            // Forward new game log entries to clients
            auto& serverLog = world.engine().getGameLog();
            auto serverEntries = serverLog.getEntries();
            if (serverEntries.size() > serverLogSentIndex) {
                auto logMsg = serializeGameLogBatch(
                    serverEntries, serverLogSentIndex, serverEntries.size());
                bridge.broadcastMessage(logMsg);
                serverLogSentIndex = serverEntries.size();
            }
        }

        // ── Client: inject remote game log entries into local log ────
        if (networkMode == NetworkMode::Client) {
            std::lock_guard<std::mutex> lk(clientLogMtx);
            for (auto& e : pendingLogEntries) {
                world.engine().getGameLog().log(
                    e.source, e.location, e.sense, e.message);
            }
            pendingLogEntries.clear();
        }

        // Process new battle log entries: print to console + create indicators
        if (networkMode != NetworkMode::Compute) {
            auto& log = world.engine().getGameLog();
            auto entries = log.getEntries();

            if (entries.size() > lastLogIndex) {
                COORD playerPos{0.0, 0.0, 0.0};
                double camYaw = 0.0;
                if (playerCtrlPtr) {
                    auto playerIt = positions.find("Player");
                    playerPos = (playerIt != positions.end())
                        ? playerIt->second : COORD{0.0, 0.0, 0.0};
                    camYaw = playerCtrlPtr->yaw();
                }

                for (size_t i = lastLogIndex; i < entries.size(); ++i) {
                    std::cout << entries[i].format() << "\n";

                    // Skip reaction entries ("Heard: ...") — only show
                    // indicators for original communications.
                    bool isReaction = entries[i].message.rfind("Heard:", 0) == 0;

                    // Create a 3D floating indicator at the source's position
                    auto it = positions.find(entries[i].source);
                    if (it != positions.end() && !isReaction) {
                        senseIndicators.addIndicator(it->second, entries[i].sense);

                        // HUD ping: compute bearing relative to camera
                        double dx = it->second[0] - playerPos[0];
                        double dz = it->second[2] - playerPos[2];
                        double distSq = dx * dx + dz * dz;
                        if (distSq > 1.0 && entries[i].source != "Player") {
                            double worldAngle = std::atan2(dz, dx);
                            float bearing = static_cast<float>(worldAngle - camYaw);
                            senseIndicators.addHudPing(bearing, entries[i].sense);
                        }
                    }
                }
                lastLogIndex = entries.size();
            }
        }

        inputMap.endFrame();

        // Headless / Compute: sleep to target ~60 ticks per second
        if (!hasDisplay) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    bridge.stop();
    world.engine().stop();
    engineThread.join();

    std::cout << "BattleGrid simulation ended.\n";
    return EXIT_SUCCESS;
}
