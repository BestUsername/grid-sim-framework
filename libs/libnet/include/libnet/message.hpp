#ifndef GRID_NET_MESSAGE_HPP_INCLUDED
#define GRID_NET_MESSAGE_HPP_INCLUDED

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace grid::net {

/// Message types exchanged between networked simulations.
enum class MessageType : uint8_t {
    /// Agent position/state snapshot (server → client).
    AgentState = 1,
    /// Input event forwarded from client to server.
    InputEvent = 2,
    /// An event to publish on the remote EventBus.
    BusEvent   = 3,
    /// Simulation control (pause, resume, stop).
    Control    = 4,
    /// Terrain data (server → client on connect).
    TerrainData = 5,
    /// Batch of game log entries (server → client).
    GameLogBatch = 6,
    /// Compute node registration (compute → server on connect).
    ComputeRegister = 7,
    /// Agent assignment (server → compute node on registration).
    AgentAssignment = 8,
    /// Server-authored, idempotent physics outcome for a compute-owned entity.
    ContactDecision = 9,
    /// Initial wire-protocol version negotiation.
    ProtocolHello = 10,
};

/**
 * @brief A length-prefixed network message.
 *
 * Wire format (little-endian):
 *   [4 bytes: payload length] [1 byte: MessageType] [N bytes: payload]
 *
 * The payload is opaque bytes whose interpretation depends on the
 * MessageType.  Higher-level code (serializers) pack and unpack
 * domain objects into the payload.
 */
class Message {
public:
    static constexpr size_t kHeaderSize = 5; // 4 (length) + 1 (type)

    Message() = default;

    Message(MessageType type, std::vector<uint8_t> payload)
        : m_type(type), m_payload(std::move(payload)) {}

    MessageType type() const { return m_type; }
    const std::vector<uint8_t>& payload() const { return m_payload; }

    /// Serialize to wire format (header + payload).
    std::vector<uint8_t> serialize() const {
        uint32_t len = static_cast<uint32_t>(m_payload.size());
        std::vector<uint8_t> buf(kHeaderSize + len);
        std::memcpy(buf.data(), &len, 4);
        buf[4] = static_cast<uint8_t>(m_type);
        if (len > 0) {
            std::memcpy(buf.data() + kHeaderSize, m_payload.data(), len);
        }
        return buf;
    }

    /// Deserialize from a buffer that contains at least kHeaderSize bytes.
    /// Returns the total bytes consumed (header + payload).
    /// Throws if the buffer is too small.
    static Message deserialize(const uint8_t* data, size_t available,
                               size_t& bytesConsumed) {
        if (available < kHeaderSize) {
            throw std::runtime_error("Message::deserialize: insufficient header bytes");
        }
        uint32_t len = 0;
        std::memcpy(&len, data, 4);
        if (len > 1024 * 1024) { // 1 MB sanity limit
            throw std::runtime_error("Message::deserialize: payload too large");
        }
        size_t total = kHeaderSize + len;
        if (available < total) {
            throw std::runtime_error("Message::deserialize: incomplete payload");
        }
        auto type = static_cast<MessageType>(data[4]);
        std::vector<uint8_t> payload(data + kHeaderSize, data + total);
        bytesConsumed = total;
        return Message(type, std::move(payload));
    }

private:
    MessageType m_type = MessageType::AgentState;
    std::vector<uint8_t> m_payload;
};

} // namespace grid::net

#endif // GRID_NET_MESSAGE_HPP_INCLUDED
