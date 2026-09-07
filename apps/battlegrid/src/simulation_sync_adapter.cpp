#include "simulation_sync_adapter.hpp"

#include <utility>

namespace battlegrid {

namespace {

void mergeInput(grid::net::InputSnapshot& pending, const grid::net::InputSnapshot& incoming)
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

} // namespace

void SimulationSyncAdapter::onMessage(const SessionPtr& session, grid::net::Message message)
{
    if (message.type() == grid::net::MessageType::ComputeRegister) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingComputeNodes.push_back(session);
        return;
    }
    onMessage(session.get(), std::move(message));
}

void SimulationSyncAdapter::onMessage(Session* session, grid::net::Message message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (message.type()) {
    case grid::net::MessageType::InputEvent:
        if (message.payload().size() == grid::net::InputSnapshot::kSerializedSize) {
            mergeInput(m_remoteInputs[session], grid::net::deserializeInputSnapshot(message));
        }
        break;
    case grid::net::MessageType::AgentState:
        for (auto& snapshot : grid::net::deserializeAgentStates(message)) {
            if (m_authority.acceptsSnapshot(snapshot.name, session)) {
                m_computeSnapshots[snapshot.name] = std::move(snapshot);
            }
        }
        break;
    default:
        break;
    }
}

void SimulationSyncAdapter::queueConnection(const SessionPtr& session)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingConnections.push_back(session);
}

void SimulationSyncAdapter::queueDisconnection(const SessionPtr& session)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_remoteInputs.erase(session.get());
    m_pendingDisconnections.push_back(session.get());
}

std::vector<SimulationSyncAdapter::SessionPtr> SimulationSyncAdapter::takePendingConnections()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::exchange(m_pendingConnections, {});
}

std::vector<SimulationSyncAdapter::SessionPtr> SimulationSyncAdapter::takePendingComputeNodes()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::exchange(m_pendingComputeNodes, {});
}

std::vector<SimulationSyncAdapter::Session*> SimulationSyncAdapter::takePendingDisconnections()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::exchange(m_pendingDisconnections, {});
}

std::unordered_map<SimulationSyncAdapter::Session*, grid::net::InputSnapshot>
SimulationSyncAdapter::takeRemoteInputs()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::exchange(m_remoteInputs, {});
}

std::vector<grid::net::AgentSnapshot> SimulationSyncAdapter::takeComputeSnapshots()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<grid::net::AgentSnapshot> snapshots;
    snapshots.reserve(m_computeSnapshots.size());
    for (const auto& [name, snapshot] : m_computeSnapshots) {
        snapshots.push_back(snapshot);
    }
    return snapshots;
}

uint64_t SimulationSyncAdapter::assign(const std::string& entityName, Session* owner)
{
    return m_authority.assign(entityName, owner);
}

std::vector<std::string> SimulationSyncAdapter::reclaim(Session* owner)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto reclaimed = m_authority.releaseOwner(owner);
    for (const auto& entityName : reclaimed) {
        m_computeSnapshots.erase(entityName);
    }
    return reclaimed;
}

bool SimulationSyncAdapter::acceptsSnapshot(const std::string& entityName, Session* owner) const
{
    return m_authority.acceptsSnapshot(entityName, owner);
}

bool SimulationSyncAdapter::isRemoteOwned(const std::string& entityName) const
{
    return m_authority.isRemoteOwned(entityName);
}

uint64_t SimulationSyncAdapter::epoch(const std::string& entityName) const
{
    return m_authority.epoch(entityName);
}

} // namespace battlegrid
