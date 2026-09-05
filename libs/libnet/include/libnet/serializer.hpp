#ifndef GRID_NET_SERIALIZER_HPP_INCLUDED
#define GRID_NET_SERIALIZER_HPP_INCLUDED

#include "libnet/message.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace grid::net {

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
};

// ── Low-level pack/unpack helpers ───────────────────────────────────────

namespace detail {

inline void packString(std::vector<uint8_t>& buf, const std::string& s) {
    uint16_t len = static_cast<uint16_t>(s.size());
    buf.resize(buf.size() + 2 + len);
    std::memcpy(buf.data() + buf.size() - 2 - len, &len, 2);
    std::memcpy(buf.data() + buf.size() - len, s.data(), len);
}

inline std::string unpackString(const uint8_t*& p) {
    uint16_t len = 0;
    std::memcpy(&len, p, 2);
    p += 2;
    std::string s(reinterpret_cast<const char*>(p), len);
    p += len;
    return s;
}

inline void packDouble(std::vector<uint8_t>& buf, double v) {
    size_t off = buf.size();
    buf.resize(off + sizeof(double));
    std::memcpy(buf.data() + off, &v, sizeof(double));
}

inline double unpackDouble(const uint8_t*& p) {
    double v = 0;
    std::memcpy(&v, p, sizeof(double));
    p += sizeof(double);
    return v;
}

inline void packUint8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

inline uint8_t unpackUint8(const uint8_t*& p) {
    uint8_t v = *p;
    ++p;
    return v;
}

inline void packFloat(std::vector<uint8_t>& buf, float v) {
    size_t off = buf.size();
    buf.resize(off + sizeof(float));
    std::memcpy(buf.data() + off, &v, sizeof(float));
}

inline float unpackFloat(const uint8_t*& p) {
    float v = 0;
    std::memcpy(&v, p, sizeof(float));
    p += sizeof(float);
    return v;
}

} // namespace detail

// ── AgentSnapshot serialization ─────────────────────────────────────────

/**
 * @brief Serialize a batch of agent snapshots into a Message payload.
 *
 * Wire format per agent:
 *   [string name] [double x] [double y] [double z] [double health]
 *   [double yaw] [uint8 entityType] [uint8 faction] [uint8 dead]
 *   [string driverName]
 * Prefixed with a uint16_t agent count.
 */
inline Message serializeAgentStates(const std::vector<AgentSnapshot>& agents) {
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
    }
    return Message(MessageType::AgentState, std::move(buf));
}

inline std::vector<AgentSnapshot> deserializeAgentStates(const Message& msg) {
    const uint8_t* p = msg.payload().data();
    uint16_t count = 0;
    std::memcpy(&count, p, 2);
    p += 2;
    std::vector<AgentSnapshot> agents(count);
    for (uint16_t i = 0; i < count; ++i) {
        agents[i].name = detail::unpackString(p);
        agents[i].position[0] = detail::unpackDouble(p);
        agents[i].position[1] = detail::unpackDouble(p);
        agents[i].position[2] = detail::unpackDouble(p);
        agents[i].health = detail::unpackDouble(p);
        agents[i].yaw = detail::unpackDouble(p);
        agents[i].entityType = detail::unpackUint8(p);
        agents[i].faction = detail::unpackUint8(p);
        agents[i].dead = detail::unpackUint8(p) != 0;
        agents[i].driverName = detail::unpackString(p);
    }
    return agents;
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
    const uint8_t* p = msg.payload().data();
    KeyEventData d;
    d.key = detail::unpackUint8(p);
    d.action = detail::unpackUint8(p);
    d.modifiers = detail::unpackUint8(p);
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
    const uint8_t* p = msg.payload().data();
    InputSnapshot in;
    in.moveX      = detail::unpackFloat(p);
    in.moveZ      = detail::unpackFloat(p);
    in.sprint     = detail::unpackFloat(p);
    in.lookDeltaX = detail::unpackFloat(p);
    in.lookDeltaY = detail::unpackFloat(p);
    in.zoomDelta  = detail::unpackFloat(p);
    in.lookAxisX  = detail::unpackFloat(p);
    in.lookAxisY  = detail::unpackFloat(p);
    uint8_t flags = detail::unpackUint8(p);
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
    const uint8_t* p = msg.payload().data();
    return static_cast<ControlCode>(detail::unpackUint8(p));
}

// ── Player assignment (server → client on connect) ──────────────────────

/// Tell the client which soldier name it controls.
inline Message serializePlayerAssignment(const std::string& name) {
    std::vector<uint8_t> buf;
    detail::packString(buf, name);
    return Message(MessageType::BusEvent, std::move(buf));
}

inline std::string deserializePlayerAssignment(const Message& msg) {
    const uint8_t* p = msg.payload().data();
    return detail::unpackString(p);
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

} // namespace grid::net

#endif // GRID_NET_SERIALIZER_HPP_INCLUDED
