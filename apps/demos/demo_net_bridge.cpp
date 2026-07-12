/**
 * @file demo_net_bridge.cpp
 * @brief Demonstrates two simulation engines communicating over TCP.
 *
 * Launches a server engine with a bouncing agent and a client engine that
 * receives agent-state snapshots from the server. The
 * client prints every position update it receives, proving that the
 * two engines are exchanging data over the network bridge.
 *
 * Run:  ./demo_net_bridge
 *       (runs both server and client in one process on localhost)
 */

#include "libnet/network_bridge.hpp"
#include "libnet/serializer.hpp"
#include "libnet/message.hpp"

#include "libsim/base_engine.hpp"
#include "libsim/base_agent.hpp"
#include "libsim/types.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using NUMBER_TYPE = int;
constexpr int NUM_DIMENSIONS = 2;

using Coord  = grid::libsim::VectX<NUMBER_TYPE, NUM_DIMENSIONS>;
using Engine = grid::libsim::BaseEngine<NUMBER_TYPE, NUM_DIMENSIONS>;
using Agent  = grid::libsim::BaseAgent<NUMBER_TYPE, NUM_DIMENSIONS>;

// ── A simple bouncing agent ─────────────────────────────────────────────

class BouncingAgent : public Agent {
public:
    BouncingAgent(Engine& engine, Coord start, const std::string& name,
                  int bound, double speed)
        : Agent(engine, start, name)
        , m_bound(bound), m_speed(speed), m_dir(1)
    {}

    void start() override {}

    void update(grid::libsim::DeltaType dt) override {
        auto loc = this->location();
        int step = static_cast<int>(m_speed * dt.count());
        if (step < 1) step = 1;
        loc[0] += step * m_dir;
        if (loc[0] >= m_bound) { loc[0] = m_bound; m_dir = -1; }
        if (loc[0] <= 0)       { loc[0] = 0;       m_dir =  1; }
        this->set_location(loc);
    }

    void on_event(grid::libevent::Event*) override {}

private:
    int m_bound;
    double m_speed;
    int m_dir;
};

// ── Main ────────────────────────────────────────────────────────────────

int main() {
    constexpr uint16_t kPort = 9742;
    constexpr double kDefaultHealth = 100.0;

    // ── Server engine ────────────────────────────────────────────────
    Engine serverEngine;
    auto bouncer = std::make_shared<BouncingAgent>(
        serverEngine, Coord{0, 5}, "Bouncer", 40, 15.0);
    serverEngine.addAgent(bouncer);

    std::cout << "[server] Starting engine + network bridge on port "
              << kPort << "\n";

    grid::net::NetworkBridge server;
    server.hostServer(kPort, [](grid::net::Message msg) {
        // Handle incoming client messages (e.g. input events)
        std::cout << "[server] Received message type "
                  << static_cast<int>(msg.type()) << "\n";
    });

    auto engineThread = serverEngine.run();

    // Give the server a moment to bind.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ── Client ───────────────────────────────────────────────────────
    std::cout << "[client] Connecting to localhost:" << kPort << "\n";

    grid::net::NetworkBridge client;
    client.connectToServer("127.0.0.1", kPort, [](grid::net::Message msg) {
        if (msg.type() == grid::net::MessageType::AgentState) {
            auto agents = grid::net::deserializeAgentStates(msg);
            for (auto& a : agents) {
                std::cout << "[client] Remote agent \"" << a.name
                          << "\" at (" << a.position[0]
                          << ", " << a.position[1]
                          << ", " << a.position[2]
                          << ") health=" << a.health << "\n";
            }
        }
    });

    // ── Simulation loop ──────────────────────────────────────────────
    std::cout << "[demo]   Running for 3 seconds...\n";

    for (int tick = 0; tick < 30; ++tick) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Server snapshots its agents and broadcasts
        auto agents = serverEngine.getAllAgents();
        std::vector<grid::net::AgentSnapshot> snapshots;
        for (auto& a : agents) {
            grid::net::AgentSnapshot snap;
            snap.name = a->name();
            auto loc = a->location();
            for (size_t d = 0; d < NUM_DIMENSIONS && d < 3; ++d) {
                snap.position[d] = static_cast<double>(loc[d]);
            }
            snap.health = kDefaultHealth;
            snapshots.push_back(std::move(snap));
        }
        server.broadcastAgentStates(snapshots);
    }

    // ── Shutdown ─────────────────────────────────────────────────────
    std::cout << "[demo]   Done. Shutting down.\n";
    client.stop();
    server.stop();
    serverEngine.stop();
    engineThread.join();

    return 0;
}
