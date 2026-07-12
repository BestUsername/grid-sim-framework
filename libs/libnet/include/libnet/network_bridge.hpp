#ifndef GRID_NET_NETWORK_BRIDGE_HPP_INCLUDED
#define GRID_NET_NETWORK_BRIDGE_HPP_INCLUDED

#include "libnet/message.hpp"
#include "libnet/serializer.hpp"
#include "libnet/tcp_client.hpp"
#include "libnet/tcp_server.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace grid::net {

/**
 * @brief Bridges a local EventBus with a remote simulation over TCP.
 *
 * Two modes:
 *  - **Server** — runs the authoritative simulation, accepts one or
 *    more clients, broadcasts agent state snapshots, and receives
 *    remote input events.
 *  - **Client** — connects to a server, sends local input events,
 *    and receives agent state updates to drive a local display.
 *
 * Usage (server side):
 * @code
 *   grid::net::NetworkBridge bridge;
 *   bridge.hostServer(port, [&](Message msg) {
 *       // handle incoming client messages (e.g. input)
 *   });
 *   // In your simulation loop:
 *   bridge.broadcastAgentStates(snapshots);
 * @endcode
 *
 * Usage (client side):
 * @code
 *   grid::net::NetworkBridge bridge;
 *   bridge.connectToServer(host, port, [&](Message msg) {
 *       // handle incoming server messages (e.g. agent state)
 *   });
 *   // When the user presses a key:
 *   bridge.sendToServer(serializeKeyEvent(key, action, mods));
 * @endcode
 */
class NetworkBridge {
public:
    using IncomingHandler    = std::function<void(Message)>;
    using SessionHandler     = std::function<void(std::shared_ptr<Session>, Message)>;
    using ConnectHandler     = std::function<void(std::shared_ptr<Session>)>;
    using DisconnectHandler  = std::function<void(std::shared_ptr<Session>)>;

    NetworkBridge() = default;
    ~NetworkBridge();

    NetworkBridge(const NetworkBridge&) = delete;
    NetworkBridge& operator=(const NetworkBridge&) = delete;

    // ── Server mode ─────────────────────────────────────────────────

    /// Start listening (simple handler, session info discarded).
    void hostServer(uint16_t port, IncomingHandler onClientMessage);

    /// Start listening with per-session callbacks.
    void hostServer(uint16_t port, SessionHandler onMsg,
                    ConnectHandler onConnect = nullptr,
                    DisconnectHandler onDisconnect = nullptr);

    /// Broadcast agent state to all connected clients.
    void broadcastAgentStates(const std::vector<AgentSnapshot>& snapshots);

    /// Send an arbitrary message to all clients.
    void broadcastMessage(const Message& msg);

    size_t clientCount() const;

    // ── Client mode ─────────────────────────────────────────────────

    /// Connect to a remote server.
    void connectToServer(const std::string& host, uint16_t port,
                         IncomingHandler onServerMessage);

    /// Send a message to the server (client mode only).
    void sendToServer(const Message& msg);

    // ── Common ──────────────────────────────────────────────────────

    bool isServer() const;
    bool isRunning() const;

    void stop();

private:
    std::unique_ptr<TcpServer> m_server;
    std::unique_ptr<TcpClient> m_client;
    std::atomic<bool> m_isServer{false};
    std::atomic<bool> m_running{false};
};

} // namespace grid::net

#endif // GRID_NET_NETWORK_BRIDGE_HPP_INCLUDED
