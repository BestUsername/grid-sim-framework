#include "libnet/tcp_client.hpp"

#include <cstring>
#include <utility>

namespace {

constexpr uint32_t kMaxPayloadSize = 1024 * 1024;
constexpr size_t kMaxPendingWrites = 256;

} // namespace

namespace grid::net {

TcpClient::TcpClient(const std::string& host, uint16_t port, MessageHandler onMessage)
    : m_onMessage(std::move(onMessage))
    , m_socket(m_ioc)
    , m_writeStrand(m_socket.get_executor())
{
    tcp::resolver resolver(m_ioc);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    boost::asio::connect(m_socket, endpoints);
}

void TcpClient::start()
{
    asyncReadHeader();
    m_thread = std::thread([this] { m_ioc.run(); });
}

void TcpClient::send(const Message& msg)
{
    auto wire = msg.serialize();
    const bool isSnapshot = msg.type() == MessageType::AgentState;
    boost::asio::post(m_writeStrand, [this, wire = std::move(wire), isSnapshot]() mutable {
        if (m_closed) {
            return;
        }

        if (isSnapshot
            && m_writeQueue.size() > 1
            && m_writeQueue.back()[Message::kHeaderSize - 1]
                == static_cast<uint8_t>(MessageType::AgentState)) {
            m_writeQueue.back() = std::move(wire);
            return;
        }
        if (m_writeQueue.size() >= kMaxPendingWrites) {
            stop();
            return;
        }

        const bool writeInProgress = !m_writeQueue.empty();
        m_writeQueue.push_back(std::move(wire));
        if (!writeInProgress) {
            asyncWrite();
        }
    });
}

void TcpClient::stop()
{
    if (m_closed.exchange(true)) {
        return;
    }
    m_ioc.stop();
    if (m_thread.joinable()) {
        if (m_thread.get_id() == std::this_thread::get_id()) {
            m_thread.detach();
        } else {
            m_thread.join();
        }
    }
    boost::system::error_code error;
    m_socket.close(error);
}

bool TcpClient::isConnected() const
{
    return m_socket.is_open();
}

void TcpClient::asyncReadHeader()
{
    m_headerBuf.resize(Message::kHeaderSize);
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_headerBuf),
        [this](boost::system::error_code error, size_t) {
            if (error) {
                return;
            }

            uint32_t payload_length = 0;
            std::memcpy(&payload_length, m_headerBuf.data(), sizeof(payload_length));
            if (payload_length > kMaxPayloadSize) {
                return;
            }

            asyncReadPayload(payload_length);
        });
}

void TcpClient::asyncReadPayload(uint32_t payloadLen)
{
    m_payloadBuf.resize(payloadLen);
    if (payloadLen == 0) {
        deliverMessage();
        return;
    }

    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_payloadBuf),
        [this](boost::system::error_code error, size_t) {
            if (error) {
                return;
            }
            deliverMessage();
        });
}

void TcpClient::deliverMessage()
{
    std::vector<uint8_t> full(m_headerBuf.begin(), m_headerBuf.end());
    full.insert(full.end(), m_payloadBuf.begin(), m_payloadBuf.end());

    size_t consumed = 0;
    try {
        auto msg = Message::deserialize(full.data(), full.size(), consumed);
        if (m_onMessage) {
            m_onMessage(std::move(msg));
        }
    } catch (...) {
        return;
    }

    asyncReadHeader();
}

void TcpClient::asyncWrite()
{
    if (m_writeQueue.empty()) {
        return;
    }

    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(m_writeQueue.front()),
        boost::asio::bind_executor(m_writeStrand, [this](boost::system::error_code error, size_t) {
            if (error) {
                stop();
                return;
            }

            m_writeQueue.pop_front();

            asyncWrite();
        }));
}

} // namespace grid::net
