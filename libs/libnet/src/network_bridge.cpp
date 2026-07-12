#include "libnet/network_bridge.hpp"

#include <utility>

namespace grid::net {

NetworkBridge::~NetworkBridge()
{
    stop();
}

void NetworkBridge::hostServer(uint16_t port, IncomingHandler onClientMessage)
{
    m_server = std::make_unique<TcpServer>(
        port,
        [handler = std::move(onClientMessage)](std::shared_ptr<Session>, Message msg) {
            handler(std::move(msg));
        });
    m_server->start();
    m_isServer = true;
    m_running = true;
}

void NetworkBridge::hostServer(uint16_t port, SessionHandler onMsg,
                               ConnectHandler onConnect, DisconnectHandler onDisconnect)
{
    m_server = std::make_unique<TcpServer>(
        port, std::move(onMsg), std::move(onConnect), std::move(onDisconnect));
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
    m_client = std::make_unique<TcpClient>(
        host, port,
        [handler = std::move(onServerMessage)](Message msg) {
            handler(std::move(msg));
        });
    m_client->start();
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
