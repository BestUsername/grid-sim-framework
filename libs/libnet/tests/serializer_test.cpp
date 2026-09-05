#include <gtest/gtest.h>
#include "libnet/serializer.hpp"

using namespace grid::net;

TEST(SerializerTest, AgentStateRoundTrip) {
    std::vector<AgentSnapshot> agents = {
        {"soldier_1", {1.0, 2.0, 3.0}, 100.0, 1.57, 0, 0, false, ""},
        {"civilian_2", {-5.5, 0.0, 10.1}, 50.0, 0.0, 1, 2, true, ""},
        {"humvee_1", {10.0, 0.0, 20.0}, 500.0, 3.14, 2, 0, false, "soldier_1"},
    };

    auto msg = serializeAgentStates(agents);
    EXPECT_EQ(msg.type(), MessageType::AgentState);

    auto decoded = deserializeAgentStates(msg);
    ASSERT_EQ(decoded.size(), 3u);

    EXPECT_EQ(decoded[0].name, "soldier_1");
    EXPECT_DOUBLE_EQ(decoded[0].position[0], 1.0);
    EXPECT_DOUBLE_EQ(decoded[0].position[1], 2.0);
    EXPECT_DOUBLE_EQ(decoded[0].position[2], 3.0);
    EXPECT_DOUBLE_EQ(decoded[0].health, 100.0);
    EXPECT_DOUBLE_EQ(decoded[0].yaw, 1.57);
    EXPECT_EQ(decoded[0].entityType, 0);
    EXPECT_EQ(decoded[0].faction, 0);
    EXPECT_FALSE(decoded[0].dead);
    EXPECT_EQ(decoded[0].driverName, "");

    EXPECT_EQ(decoded[1].name, "civilian_2");
    EXPECT_DOUBLE_EQ(decoded[1].position[0], -5.5);
    EXPECT_DOUBLE_EQ(decoded[1].health, 50.0);
    EXPECT_EQ(decoded[1].entityType, 1);
    EXPECT_EQ(decoded[1].faction, 2);
    EXPECT_TRUE(decoded[1].dead);

    EXPECT_EQ(decoded[2].name, "humvee_1");
    EXPECT_EQ(decoded[2].driverName, "soldier_1");
    EXPECT_DOUBLE_EQ(decoded[2].yaw, 3.14);
}

TEST(SerializerTest, EmptyAgentList) {
    auto msg = serializeAgentStates({});
    auto decoded = deserializeAgentStates(msg);
    EXPECT_TRUE(decoded.empty());
}

TEST(SerializerTest, KeyEventRoundTrip) {
    auto msg = serializeKeyEvent(42, 1, 3);
    auto decoded = deserializeKeyEvent(msg);

    EXPECT_EQ(decoded.key, 42);
    EXPECT_EQ(decoded.action, 1);
    EXPECT_EQ(decoded.modifiers, 3);
}

TEST(SerializerTest, InputSnapshotRoundTrip) {
    InputSnapshot in;
    in.moveX = -0.5f;
    in.moveZ = 1.0f;
    in.sprint = 0.75f;
    in.lookDeltaX = 12.5f;
    in.lookDeltaY = -3.0f;
    in.zoomDelta = 0.5f;
    in.lookAxisX = 0.8f;
    in.lookAxisY = -0.2f;
    in.jump = true;
    in.interact = false;
    in.shout = true;
    in.toggleCamera = false;

    auto msg = serializeInputSnapshot(in);
    EXPECT_EQ(msg.type(), MessageType::InputEvent);

    auto decoded = deserializeInputSnapshot(msg);
    EXPECT_FLOAT_EQ(decoded.moveX, -0.5f);
    EXPECT_FLOAT_EQ(decoded.moveZ, 1.0f);
    EXPECT_FLOAT_EQ(decoded.sprint, 0.75f);
    EXPECT_FLOAT_EQ(decoded.lookDeltaX, 12.5f);
    EXPECT_FLOAT_EQ(decoded.lookDeltaY, -3.0f);
    EXPECT_FLOAT_EQ(decoded.zoomDelta, 0.5f);
    EXPECT_FLOAT_EQ(decoded.lookAxisX, 0.8f);
    EXPECT_FLOAT_EQ(decoded.lookAxisY, -0.2f);
    EXPECT_TRUE(decoded.jump);
    EXPECT_FALSE(decoded.interact);
    EXPECT_TRUE(decoded.shout);
    EXPECT_FALSE(decoded.toggleCamera);
}

TEST(SerializerTest, InputSnapshotAllButtons) {
    InputSnapshot in;
    in.jump = true;
    in.interact = true;
    in.shout = true;
    in.toggleCamera = true;

    auto decoded = deserializeInputSnapshot(serializeInputSnapshot(in));
    EXPECT_TRUE(decoded.jump);
    EXPECT_TRUE(decoded.interact);
    EXPECT_TRUE(decoded.shout);
    EXPECT_TRUE(decoded.toggleCamera);
}

TEST(SerializerTest, RejectsInvalidInputSnapshotPayloadSize) {
    const Message truncated(MessageType::InputEvent, {0, 0, 0, 0});
    EXPECT_THROW(deserializeInputSnapshot(truncated), std::invalid_argument);
}

TEST(SerializerTest, ControlRoundTrip) {
    auto msg = serializeControl(ControlCode::Pause);
    auto decoded = deserializeControl(msg);
    EXPECT_EQ(decoded, ControlCode::Pause);

    msg = serializeControl(ControlCode::Stop);
    decoded = deserializeControl(msg);
    EXPECT_EQ(decoded, ControlCode::Stop);
}
