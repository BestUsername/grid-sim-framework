#ifndef GRID_NET_TCP_CLIENT_HPP_INCLUDED
#define GRID_NET_TCP_CLIENT_HPP_INCLUDED

#include "libnet/message.hpp"

#include <boost/asio.hpp>

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace grid::net {

using boost::asio::ip::tcp;

/**
 * @brief TCP client that connects to a TcpServer and exchanges Messages.
 *
 * Runs a Boost.Asio io_context on a background thread.  Incoming
 * messages are delivered via the onMessage callback.
 */
class TcpClient {
public:
    using MessageHandler = std::function<void(Message)>;

    TcpClient(const std::string& host, uint16_t port,
              MessageHandler onMessage);

    /// Start the I/O thread.  Call once after construction.
    void start();

    /// Send a message to the server.
    void send(const Message& msg);

    void stop();

    bool isConnected() const;

private:
    void asyncReadHeader();

    void asyncReadPayload(uint32_t payloadLen);

    void deliverMessage();

    void asyncWrite();

    boost::asio::io_context m_ioc;
    tcp::socket m_socket;
    boost::asio::strand<tcp::socket::executor_type> m_writeStrand;
    std::thread m_thread;

    MessageHandler m_onMessage;

    std::vector<uint8_t> m_headerBuf;
    std::vector<uint8_t> m_payloadBuf;

    std::atomic_bool m_closed{false};
    std::deque<std::vector<uint8_t>> m_writeQueue;
};

} // namespace grid::net

#endif // GRID_NET_TCP_CLIENT_HPP_INCLUDED
