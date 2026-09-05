#ifndef GRID_NET_TCP_SERVER_HPP_INCLUDED
#define GRID_NET_TCP_SERVER_HPP_INCLUDED

#include "libnet/message.hpp"

#include <boost/asio.hpp>

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace grid::net {

using boost::asio::ip::tcp;

/**
 * @brief A single TCP session with a connected peer.
 *
 * Manages async read/write of length-prefixed Messages.
 * Owned by TcpServer — one Session per connected client.
 */
class Session : public std::enable_shared_from_this<Session> {
public:
    using MessageHandler = std::function<void(std::shared_ptr<Session>, Message)>;
    using CloseHandler   = std::function<void(std::shared_ptr<Session>)>;

    Session(tcp::socket socket, MessageHandler onMessage, CloseHandler onClose);

    void start();

    void send(const Message& msg);

    bool isOpen() const;

    void close();

private:
    void asyncReadHeader();

    void asyncReadPayload(uint32_t payloadLen);

    void deliverMessage();

    void asyncWrite();

    void handleError();

    tcp::socket m_socket;
    boost::asio::strand<tcp::socket::executor_type> m_writeStrand;
    MessageHandler m_onMessage;
    CloseHandler   m_onClose;

    std::vector<uint8_t> m_headerBuf;
    std::vector<uint8_t> m_payloadBuf;

    std::mutex m_writeMutex;
    std::deque<std::vector<uint8_t>> m_writeQueue;
};

/**
 * @brief TCP server that accepts connections and manages Sessions.
 *
 * Runs a Boost.Asio io_context on a background thread.  Incoming
 * messages are delivered via the onMessage callback.
 */
class TcpServer {
public:
    using MessageHandler    = Session::MessageHandler;
    using ConnectHandler    = std::function<void(std::shared_ptr<Session>)>;
    using DisconnectHandler = std::function<void(std::shared_ptr<Session>)>;

    TcpServer(uint16_t port, MessageHandler onMessage,
              ConnectHandler onConnect = nullptr,
              DisconnectHandler onDisconnect = nullptr);

    /// Start the I/O thread.  Call once after construction.
    void start();

    /// Stop accepting and close all sessions.
    void stop();

    /// Broadcast a message to all connected clients.
    void broadcast(const Message& msg);

    size_t clientCount() const;

private:
    void asyncAccept();

    void removeSession(std::shared_ptr<Session> s);

    boost::asio::io_context m_ioc;
    tcp::acceptor m_acceptor;
    std::thread m_thread;

    MessageHandler m_onMessage;
    ConnectHandler m_onConnect;
    DisconnectHandler m_onDisconnect;

    mutable std::mutex m_sessionsMutex;
    std::vector<std::shared_ptr<Session>> m_sessions;
};

} // namespace grid::net

#endif // GRID_NET_TCP_SERVER_HPP_INCLUDED
