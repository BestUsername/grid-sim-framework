#include "libnet/tcp_server.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace {

constexpr uint32_t kMaxPayloadSize = 1024 * 1024;

} // namespace

namespace grid::net {

Session::Session(tcp::socket socket, MessageHandler onMessage, CloseHandler onClose)
    : m_socket(std::move(socket))
    , m_onMessage(std::move(onMessage))
    , m_onClose(std::move(onClose))
{
}

void Session::start()
{
    asyncReadHeader();
}

void Session::send(const Message& msg)
{
    auto wire = msg.serialize();
    bool write_in_progress = false;
    {
        std::lock_guard<std::mutex> lock(m_writeMutex);
        write_in_progress = !m_writeQueue.empty();
        m_writeQueue.push_back(std::move(wire));
    }
    if (!write_in_progress) {
        asyncWrite();
    }
}

bool Session::isOpen() const
{
    return m_socket.is_open();
}

void Session::close()
{
    boost::system::error_code error;
    m_socket.shutdown(tcp::socket::shutdown_both, error);
    m_socket.close(error);
}

void Session::asyncReadHeader()
{
    auto self = shared_from_this();
    m_headerBuf.resize(Message::kHeaderSize);
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_headerBuf),
        [this, self](boost::system::error_code error, size_t) {
            if (error) {
                handleError();
                return;
            }

            uint32_t payload_length = 0;
            std::memcpy(&payload_length, m_headerBuf.data(), sizeof(payload_length));
            if (payload_length > kMaxPayloadSize) {
                handleError();
                return;
            }

            asyncReadPayload(payload_length);
        });
}

void Session::asyncReadPayload(uint32_t payloadLen)
{
    auto self = shared_from_this();
    m_payloadBuf.resize(payloadLen);
    if (payloadLen == 0) {
        deliverMessage();
        return;
    }

    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_payloadBuf),
        [this, self](boost::system::error_code error, size_t) {
            if (error) {
                handleError();
                return;
            }
            deliverMessage();
        });
}

void Session::deliverMessage()
{
    std::vector<uint8_t> full(m_headerBuf.begin(), m_headerBuf.end());
    full.insert(full.end(), m_payloadBuf.begin(), m_payloadBuf.end());

    size_t consumed = 0;
    try {
        auto msg = Message::deserialize(full.data(), full.size(), consumed);
        if (m_onMessage) {
            m_onMessage(shared_from_this(), std::move(msg));
        }
    } catch (...) {
        handleError();
        return;
    }

    asyncReadHeader();
}

void Session::asyncWrite()
{
    std::lock_guard<std::mutex> lock(m_writeMutex);
    if (m_writeQueue.empty()) {
        return;
    }

    auto self = shared_from_this();
    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(m_writeQueue.front()),
        [this, self](boost::system::error_code error, size_t) {
            if (error) {
                handleError();
                return;
            }

            {
                std::lock_guard<std::mutex> inner_lock(m_writeMutex);
                m_writeQueue.pop_front();
            }

            asyncWrite();
        });
}

void Session::handleError()
{
    if (m_onClose) {
        m_onClose(shared_from_this());
    }
    boost::system::error_code error;
    m_socket.close(error);
}

TcpServer::TcpServer(uint16_t port, MessageHandler onMessage, ConnectHandler onConnect,
                     DisconnectHandler onDisconnect)
    : m_acceptor(m_ioc, tcp::endpoint(tcp::v4(), port))
    , m_onMessage(std::move(onMessage))
    , m_onConnect(std::move(onConnect))
    , m_onDisconnect(std::move(onDisconnect))
{
    asyncAccept();
}

void TcpServer::start()
{
    m_thread = std::thread([this] { m_ioc.run(); });
}

void TcpServer::stop()
{
    m_ioc.stop();
    if (m_thread.joinable()) {
        m_thread.join();
    }

    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    for (auto& session : m_sessions) {
        session->close();
    }
    m_sessions.clear();
}

void TcpServer::broadcast(const Message& msg)
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    for (auto& session : m_sessions) {
        if (session->isOpen()) {
            session->send(msg);
        }
    }
}

size_t TcpServer::clientCount() const
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    return m_sessions.size();
}

void TcpServer::asyncAccept()
{
    m_acceptor.async_accept(
        [this](boost::system::error_code error, tcp::socket socket) {
            if (!error) {
                auto session = std::make_shared<Session>(
                    std::move(socket), m_onMessage,
                    [this](std::shared_ptr<Session> session_to_remove) {
                        if (m_onDisconnect) {
                            m_onDisconnect(session_to_remove);
                        }
                        removeSession(session_to_remove);
                    });

                {
                    std::lock_guard<std::mutex> lock(m_sessionsMutex);
                    m_sessions.push_back(session);
                }

                if (m_onConnect) {
                    m_onConnect(session);
                }
                session->start();
            }

            asyncAccept();
        });
}

void TcpServer::removeSession(std::shared_ptr<Session> s)
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    m_sessions.erase(std::remove(m_sessions.begin(), m_sessions.end(), s), m_sessions.end());
}

} // namespace grid::net
