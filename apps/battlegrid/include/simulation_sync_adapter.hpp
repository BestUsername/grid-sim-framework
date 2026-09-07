#ifndef BATTLEGRID_SIMULATION_SYNC_ADAPTER_HPP_INCLUDED
#define BATTLEGRID_SIMULATION_SYNC_ADAPTER_HPP_INCLUDED

#include "authority_registry.hpp"
#include "libnet/serializer.hpp"
#include "libnet/tcp_server.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace battlegrid {

/**
 * @brief Transfers validated server-side network state to the game thread.
 *
 * Network callbacks only enqueue parsed inputs and snapshots here. The game
 * thread drains the queues, performs world mutations, and assigns authority.
 */
class SimulationSyncAdapter {
public:
    using Session = grid::net::Session;
    using SessionPtr = std::shared_ptr<Session>;

    void onMessage(const SessionPtr& session, grid::net::Message message);
    void onMessage(Session* session, grid::net::Message message);
    void queueConnection(const SessionPtr& session);
    void queueDisconnection(const SessionPtr& session);

    std::vector<SessionPtr> takePendingConnections();
    std::vector<SessionPtr> takePendingComputeNodes();
    std::vector<Session*> takePendingDisconnections();
    std::unordered_map<Session*, grid::net::InputSnapshot> takeRemoteInputs();
    std::vector<grid::net::AgentSnapshot> takeComputeSnapshots();

    uint64_t assign(const std::string& entityName, Session* owner);
    std::vector<std::string> reclaim(Session* owner);
    bool acceptsSnapshot(const std::string& entityName, Session* owner) const;
    bool isRemoteOwned(const std::string& entityName) const;
    uint64_t epoch(const std::string& entityName) const;

private:
    mutable std::mutex m_mutex;
    AuthorityRegistry m_authority;
    std::vector<SessionPtr> m_pendingConnections;
    std::vector<SessionPtr> m_pendingComputeNodes;
    std::vector<Session*> m_pendingDisconnections;
    std::unordered_map<Session*, grid::net::InputSnapshot> m_remoteInputs;
    std::unordered_map<std::string, grid::net::AgentSnapshot> m_computeSnapshots;
};

} // namespace battlegrid

#endif // BATTLEGRID_SIMULATION_SYNC_ADAPTER_HPP_INCLUDED
