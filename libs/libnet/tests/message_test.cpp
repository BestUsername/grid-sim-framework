#include <gtest/gtest.h>
#include "libnet/message.hpp"

using namespace grid::net;

TEST(MessageTest, RoundTrip) {
    std::vector<uint8_t> payload = {0x01, 0x02, 0x03, 0x04};
    Message orig(MessageType::AgentState, payload);

    auto wire = orig.serialize();
    ASSERT_GE(wire.size(), Message::kHeaderSize + payload.size());

    size_t consumed = 0;
    auto decoded = Message::deserialize(wire.data(), wire.size(), consumed);

    EXPECT_EQ(consumed, wire.size());
    EXPECT_EQ(decoded.type(), MessageType::AgentState);
    EXPECT_EQ(decoded.payload(), payload);
}

TEST(MessageTest, EmptyPayload) {
    Message orig(MessageType::Control, {});
    auto wire = orig.serialize();

    size_t consumed = 0;
    auto decoded = Message::deserialize(wire.data(), wire.size(), consumed);

    EXPECT_EQ(decoded.type(), MessageType::Control);
    EXPECT_TRUE(decoded.payload().empty());
}

TEST(MessageTest, RejectsTooShortBuffer) {
    uint8_t buf[3] = {};
    size_t consumed = 0;
    EXPECT_THROW(Message::deserialize(buf, 3, consumed), std::runtime_error);
}

TEST(MessageTest, RejectsIncompletePayload) {
    // Header says 100 bytes, but we only provide 10 total
    uint32_t len = 100;
    uint8_t buf[10] = {};
    std::memcpy(buf, &len, 4);
    buf[4] = static_cast<uint8_t>(MessageType::AgentState);

    size_t consumed = 0;
    EXPECT_THROW(Message::deserialize(buf, 10, consumed), std::runtime_error);
}
