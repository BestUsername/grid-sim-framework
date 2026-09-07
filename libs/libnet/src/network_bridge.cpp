#include "libnet/network_bridge.hpp"

#include <mutex>
#include <unordered_set>
#include <utility>

namespace grid::net {

namespace {

struct ServerProtocolState {
    std::mutex mutex;
    std::unordered_set<Session*> negotiatedSessions;
};

} // namespace

NetworkBridge::~NetworkBridge()
{
    stop();
}

void NetworkBridge::hostServer(uint16_t port, IncomingHandler onClientMessage)
{
    hostServer(port,
               [handler = std::move(onClientMessage)](std::shared_ptr<Session>, Message msg) {
                   handler(std::move(msg));
               });
}

void NetworkBridge::hostServer(uint16_t port, SessionHandler onMsg,
                               ConnectHandler onConnect, DisconnectHandler onDisconnect)
{
    auto protocolState = std::make_shared<ServerProtocolState>();
    m_server = std::make_unique<TcpServer>(
        port,
        [protocolState, handler = std::move(onMsg), onConnect = std::move(onConnect)](
            std::shared_ptr<Session> session, Message msg) {
            if (msg.type() == MessageType::ProtocolHello) {
                if (deserializeProtocolHello(msg) != kProtocolVersion) {
                    session->close();
                    return;
                }

                bool newlyNegotiated = false;
                {
                    std::lock_guard<std::mutex> lock(protocolState->mutex);
                    newlyNegotiated = protocolState->negotiatedSessions.insert(session.get()).second;
                }
                if (!newlyNegotiated) {
                    session->close();
                    return;
                }

                session->send(serializeProtocolHello());
                if (onConnect) {
                    onConnect(std::move(session));
                }
                return;
            }

            {
                std::lock_guard<std::mutex> lock(protocolState->mutex);
                if (!protocolState->negotiatedSessions.contains(session.get())) {
                    session->close();
                    return;
                }
            }
            handler(std::move(session), std::move(msg));
        },
        nullptr,
        [protocolState, onDisconnect = std::move(onDisconnect)](std::shared_ptr<Session> session) {
            bool wasNegotiated = false;
            {
                std::lock_guard<std::mutex> lock(protocolState->mutex);
                wasNegotiated = protocolState->negotiatedSessions.erase(session.get()) != 0;
            }
            if (wasNegotiated && onDisconnect) {
                onDisconnect(std::move(session));
            }
        });
    m_server->start();
    m_isServer = true;
    m_running = true;
}

void NetworkBridge::broadcastAgentStates(const std::vector<AgentSnapshot>& snapshots)
{
    if (!m_server) {
        return;
    }

    auto msg = serializeAgentStates(snapshots);
    m_server->broadcast(msg);
}

void NetworkBridge::broadcastMessage(const Message& msg)
{
    if (m_server) {
        m_server->broadcast(msg);
    }
}

size_t NetworkBridge::clientCount() const
{
    return m_server ? m_server->clientCount() : 0;
}

void NetworkBridge::connectToServer(const std::string& host, uint16_t port,
                                    IncomingHandler onServerMessage)
{
    auto negotiated = std::make_shared<std::atomic_bool>(false);
    m_client = std::make_unique<TcpClient>(
        host, port,
        [this, negotiated, handler = std::move(onServerMessage)](Message msg) {
            if (!negotiated->load()) {
                if (msg.type() != MessageType::ProtocolHello
                    || deserializeProtocolHello(msg) != kProtocolVersion) {
                    m_client->close();
                    return;
                }
                negotiated->store(true);
                return;
            }
            if (msg.type() == MessageType::ProtocolHello) {
                m_client->close();
                return;
            }
            handler(std::move(msg));
        });
    m_client->start();
    m_client->send(serializeProtocolHello());
    m_isServer = false;
    m_running = true;
}

void NetworkBridge::sendToServer(const Message& msg)
{
    if (m_client) {
        m_client->send(msg);
    }
}

bool NetworkBridge::isServer() const
{
    return m_isServer;
}

bool NetworkBridge::isRunning() const
{
    return m_running;
}

void NetworkBridge::stop()
{
    m_running = false;
    if (m_server) {
        m_server->stop();
        m_server.reset();
    }
    if (m_client) {
        m_client->stop();
        m_client.reset();
    }
}

} // namespace grid::net
