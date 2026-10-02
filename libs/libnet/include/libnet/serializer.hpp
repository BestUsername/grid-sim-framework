#ifndef GRID_NET_SERIALIZER_HPP_INCLUDED
#define GRID_NET_SERIALIZER_HPP_INCLUDED

#include "libnet/message.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace grid::net {

inline constexpr uint16_t kProtocolVersion = 3;

/**
 * @brief Compact agent state snapshot sent over the wire.
 *
 * Carries the data needed to replicate a remote agent on the
 * receiving simulation: identity, spatial state, visual state,
 * and gameplay state.
 */
struct AgentSnapshot {
    std::string name;
    std::array<double, 3> position{};
    double health = 0.0;
    double yaw = 0.0;
    uint8_t entityType = 0;  ///< EntityType enum cast to uint8
    uint8_t faction = 0;     ///< Faction enum cast to uint8
    bool dead = false;
    std::string driverName;  ///< Name of mounted soldier (vehicles only)
    uint64_t ownerEpoch = 0;
    /// Authoritative presentation-only wheel centers for a land vehicle.
    bool hasWheelPresentation = false;
    std::array<std::array<double, 3>, 4> wheelPositions{};
};

// ── Low-level pack/unpack helpers ───────────────────────────────────────

namespace detail {

inline void packString(std::vector<uint8_t>& buf, const std::string& s) {
    if (s.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("string exceeds network protocol limit");
    }
    uint16_t len = static_cast<uint16_t>(s.size());
    buf.resize(buf.size() + 2 + len);
    std::memcpy(buf.data() + buf.size() - 2 - len, &len, 2);
    std::memcpy(buf.data() + buf.size() - len, s.data(), len);
}

inline void packUint64(std::vector<uint8_t>& buf, uint64_t v) {
    size_t off = buf.size();
    buf.resize(off + sizeof(v));
    std::memcpy(buf.data() + off, &v, sizeof(v));
}

inline void packDouble(std::vector<uint8_t>& buf, double v) {
    size_t off = buf.size();
    buf.resize(off + sizeof(double));
    std::memcpy(buf.data() + off, &v, sizeof(double));
}

inline void packUint8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

inline void packFloat(std::vector<uint8_t>& buf, float v) {
    size_t off = buf.size();
    buf.resize(off + sizeof(float));
    std::memcpy(buf.data() + off, &v, sizeof(float));
}

class Reader {
public:
    explicit Reader(const std::vector<uint8_t>& payload)
        : m_payload(payload)
    {
    }

    size_t remaining() const
    {
        return m_payload.size() - m_offset;
    }

    bool empty() const
    {
        return m_offset == m_payload.size();
    }

    uint8_t readUint8()
    {
        require(sizeof(uint8_t));
        return m_payload[m_offset++];
    }

    uint16_t readUint16()
    {
        return readValue<uint16_t>();
    }

    uint64_t readUint64()
    {
        return readValue<uint64_t>();
    }

    float readFloat()
    {
        const float value = readValue<float>();
        if (!std::isfinite(value)) {
            throw std::invalid_argument("network payload contains non-finite float");
        }
        return value;
    }

    double readDouble()
    {
        const double value = readValue<double>();
        if (!std::isfinite(value)) {
            throw std::invalid_argument("network payload contains non-finite double");
        }
        return value;
    }

    std::string readString()
    {
        const uint16_t length = readValue<uint16_t>();
        require(length);
        std::string value(reinterpret_cast<const char*>(m_payload.data() + m_offset), length);
        m_offset += length;
        return value;
    }

private:
    void require(size_t size) const
    {
        if (remaining() < size) {
            throw std::invalid_argument("truncated network payload");
        }
    }

    template <typename T>
    T readValue()
    {
        require(sizeof(T));
        T value{};
        std::memcpy(&value, m_payload.data() + m_offset, sizeof(T));
        m_offset += sizeof(T);
        return value;
    }

    const std::vector<uint8_t>& m_payload;
    size_t m_offset = 0;
};

} // namespace detail

// ── AgentSnapshot serialization ─────────────────────────────────────────

/**
 * @brief Serialize a batch of agent snapshots into a Message payload.
 *
 * Wire format per agent:
 *   [string name] [double x] [double y] [double z] [double health]
 *   [double yaw] [uint8 entityType] [uint8 faction] [uint8 dead]
 *   [string driverName] [uint64 ownerEpoch] [uint8 hasWheelPresentation]
 *   [[double wheelX] [double wheelY] [double wheelZ]] * 4 when present
 * Prefixed with a uint16_t agent count.
 */
inline Message serializeAgentStates(const std::vector<AgentSnapshot>& agents) {
    if (agents.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("agent batch exceeds network protocol limit");
    }
    std::vector<uint8_t> buf;
    uint16_t count = static_cast<uint16_t>(agents.size());
    buf.resize(2);
    std::memcpy(buf.data(), &count, 2);
    for (auto& a : agents) {
        detail::packString(buf, a.name);
        detail::packDouble(buf, a.position[0]);
        detail::packDouble(buf, a.position[1]);
        detail::packDouble(buf, a.position[2]);
        detail::packDouble(buf, a.health);
        detail::packDouble(buf, a.yaw);
        detail::packUint8(buf, a.entityType);
        detail::packUint8(buf, a.faction);
        detail::packUint8(buf, a.dead ? 1 : 0);
        detail::packString(buf, a.driverName);
        detail::packUint64(buf, a.ownerEpoch);
        detail::packUint8(buf, a.hasWheelPresentation ? 1 : 0);
        if (a.hasWheelPresentation) {
            for (const auto& wheel : a.wheelPositions) {
                detail::packDouble(buf, wheel[0]);
                detail::packDouble(buf, wheel[1]);
                detail::packDouble(buf, wheel[2]);
            }
        }
    }
    return Message(MessageType::AgentState, std::move(buf));
}

inline std::vector<AgentSnapshot> deserializeAgentStates(const Message& msg) {
    detail::Reader reader(msg.payload());
    const uint16_t count = reader.readUint16();
    std::vector<AgentSnapshot> agents(count);
    for (uint16_t i = 0; i < count; ++i) {
        agents[i].name = reader.readString();
        agents[i].position[0] = reader.readDouble();
        agents[i].position[1] = reader.readDouble();
        agents[i].position[2] = reader.readDouble();
        agents[i].health = reader.readDouble();
        agents[i].yaw = reader.readDouble();
        agents[i].entityType = reader.readUint8();
        agents[i].faction = reader.readUint8();
        agents[i].dead = reader.readUint8() != 0;
        agents[i].driverName = reader.readString();
        agents[i].ownerEpoch = reader.readUint64();
        agents[i].hasWheelPresentation = reader.readUint8() != 0;
        if (agents[i].hasWheelPresentation) {
            for (auto& wheel : agents[i].wheelPositions) {
                wheel[0] = reader.readDouble();
                wheel[1] = reader.readDouble();
                wheel[2] = reader.readDouble();
            }
        }
    }
    if (!reader.empty()) {
        throw std::invalid_argument("agent state payload contains trailing data");
    }
    return agents;
}

// ── Protocol negotiation ────────────────────────────────────────────────

inline Message serializeProtocolHello(uint16_t version = kProtocolVersion)
{
    std::vector<uint8_t> buf(2);
    std::memcpy(buf.data(), &version, sizeof(version));
    return Message(MessageType::ProtocolHello, std::move(buf));
}

inline uint16_t deserializeProtocolHello(const Message& msg)
{
    if (msg.type() != MessageType::ProtocolHello) {
        throw std::invalid_argument("expected protocol hello message");
    }
    detail::Reader reader(msg.payload());
    const auto version = reader.readUint16();
    if (!reader.empty()) {
        throw std::invalid_argument("protocol hello payload contains trailing data");
    }
    return version;
}

// ── InputEvent serialization (key events only for now) ──────────────────

/**
 * @brief Serialize a key event: [uint8 key] [uint8 action] [uint8 modifiers]
 */
inline Message serializeKeyEvent(uint8_t key, uint8_t action, uint8_t modifiers) {
    std::vector<uint8_t> buf;
    detail::packUint8(buf, key);
    detail::packUint8(buf, action);
    detail::packUint8(buf, modifiers);
    return Message(MessageType::InputEvent, std::move(buf));
}

struct KeyEventData {
    uint8_t key;
    uint8_t action;
    uint8_t modifiers;
};

inline KeyEventData deserializeKeyEvent(const Message& msg) {
    detail::Reader reader(msg.payload());
    KeyEventData d{reader.readUint8(), reader.readUint8(), reader.readUint8()};
    if (!reader.empty()) {
        throw std::invalid_argument("key event payload contains trailing data");
    }
    return d;
}

// ── InputSnapshot serialization (full per-frame input state) ────────────

/**
 * @brief Per-frame input state sent from a client to the server.
 *
 * Captures continuous axis values (movement, look, sprint, zoom)
 * and edge-triggered button presses (jump, interact, shout, toggle camera).
 * This maps directly to the battlegrid InputMap query interface.
 */
struct InputSnapshot {
    static constexpr size_t kSerializedSize = 8 * sizeof(float) + sizeof(uint8_t);

    // Continuous axes — persistent state from held keys / gamepad sticks
    float moveX  = 0.0f;   ///< Strafe: -1 left, +1 right
    float moveZ  = 0.0f;   ///< Forward/back: -1 fwd, +1 back
    float sprint = 0.0f;   ///< Sprint factor: 0 walk, 1 full sprint

    // Per-frame deltas — from mouse motion / scroll
    float lookDeltaX = 0.0f;
    float lookDeltaY = 0.0f;
    float zoomDelta  = 0.0f;

    // Continuous look axes — from gamepad sticks / held keys
    float lookAxisX = 0.0f;
    float lookAxisY = 0.0f;

    // Edge-triggered actions — true the frame the button was pressed
    bool jump         = false;
    bool interact     = false;
    bool shout        = false;
    bool toggleCamera = false;
};

inline Message serializeInputSnapshot(const InputSnapshot& in) {
    std::vector<uint8_t> buf;
    detail::packFloat(buf, in.moveX);
    detail::packFloat(buf, in.moveZ);
    detail::packFloat(buf, in.sprint);
    detail::packFloat(buf, in.lookDeltaX);
    detail::packFloat(buf, in.lookDeltaY);
    detail::packFloat(buf, in.zoomDelta);
    detail::packFloat(buf, in.lookAxisX);
    detail::packFloat(buf, in.lookAxisY);
    uint8_t flags = 0;
    if (in.jump)         flags |= 1;
    if (in.interact)     flags |= 2;
    if (in.shout)        flags |= 4;
    if (in.toggleCamera) flags |= 8;
    detail::packUint8(buf, flags);
    return Message(MessageType::InputEvent, std::move(buf));
}

inline InputSnapshot deserializeInputSnapshot(const Message& msg) {
    if (msg.payload().size() != InputSnapshot::kSerializedSize) {
        throw std::invalid_argument("invalid InputSnapshot payload size");
    }
    detail::Reader reader(msg.payload());
    InputSnapshot in;
    in.moveX      = reader.readFloat();
    in.moveZ      = reader.readFloat();
    in.sprint     = reader.readFloat();
    in.lookDeltaX = reader.readFloat();
    in.lookDeltaY = reader.readFloat();
    in.zoomDelta  = reader.readFloat();
    in.lookAxisX  = reader.readFloat();
    in.lookAxisY  = reader.readFloat();
    const uint8_t flags = reader.readUint8();
    in.jump         = (flags & 1) != 0;
    in.interact     = (flags & 2) != 0;
    in.shout        = (flags & 4) != 0;
    in.toggleCamera = (flags & 8) != 0;
    return in;
}

// ── Control message serialization ───────────────────────────────────────

enum class ControlCode : uint8_t {
    Pause  = 1,
    Resume = 2,
    Stop   = 3,
};

inline Message serializeControl(ControlCode code) {
    std::vector<uint8_t> buf;
    detail::packUint8(buf, static_cast<uint8_t>(code));
    return Message(MessageType::Control, std::move(buf));
}

inline ControlCode deserializeControl(const Message& msg) {
    detail::Reader reader(msg.payload());
    const auto code = static_cast<ControlCode>(reader.readUint8());
    if (!reader.empty() || (code != ControlCode::Pause && code != ControlCode::Resume
                            && code != ControlCode::Stop)) {
        throw std::invalid_argument("invalid control payload");
    }
    return code;
}

// ── Player assignment (server → client on connect) ──────────────────────

/// Tell the client which soldier name it controls.
inline Message serializePlayerAssignment(const std::string& name) {
    std::vector<uint8_t> buf;
    detail::packString(buf, name);
    return Message(MessageType::BusEvent, std::move(buf));
}

inline std::string deserializePlayerAssignment(const Message& msg) {
    detail::Reader reader(msg.payload());
    auto name = reader.readString();
    if (!reader.empty()) {
        throw std::invalid_argument("player assignment payload contains trailing data");
    }
    return name;
}

// ── Compute node registration (compute → server) ───────────────────────

inline Message serializeComputeRegister() {
    return Message(MessageType::ComputeRegister, {});
}

// ── Agent assignment (server → compute node) ────────────────────────────
// Payload format is identical to AgentState; only the MessageType differs.

inline Message serializeAgentAssignment(const std::vector<AgentSnapshot>& snapshots) {
    auto agentMsg = serializeAgentStates(snapshots);
    return Message(MessageType::AgentAssignment, agentMsg.payload());
}

inline std::vector<AgentSnapshot> deserializeAgentAssignment(const Message& msg) {
    // Payload format is the same as AgentState.
    return deserializeAgentStates(msg);
}

struct ContactDecision {
    uint64_t contactId = 0;
    uint64_t ownerEpoch = 0;
    uint64_t decisionTick = 0;
    std::string targetName;
    std::array<double, 3> impulse{};
};

inline Message serializeContactDecision(const ContactDecision& decision) {
    std::vector<uint8_t> buf;
    detail::packUint64(buf, decision.contactId);
    detail::packUint64(buf, decision.ownerEpoch);
    detail::packUint64(buf, decision.decisionTick);
    detail::packString(buf, decision.targetName);
    detail::packDouble(buf, decision.impulse[0]);
    detail::packDouble(buf, decision.impulse[1]);
    detail::packDouble(buf, decision.impulse[2]);
    return Message(MessageType::ContactDecision, std::move(buf));
}

inline ContactDecision deserializeContactDecision(const Message& msg) {
    detail::Reader reader(msg.payload());
    ContactDecision decision;
    decision.contactId = reader.readUint64();
    decision.ownerEpoch = reader.readUint64();
    decision.decisionTick = reader.readUint64();
    decision.targetName = reader.readString();
    decision.impulse[0] = reader.readDouble();
    decision.impulse[1] = reader.readDouble();
    decision.impulse[2] = reader.readDouble();
    if (!reader.empty()) {
        throw std::invalid_argument("invalid ContactDecision payload size");
    }
    return decision;
}

} // namespace grid::net

#endif // GRID_NET_SERIALIZER_HPP_INCLUDED
