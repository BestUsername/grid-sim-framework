#include "libnet/network_bridge.hpp"
#include "libnet/serializer.hpp"
#include "libnet/tcp_client.hpp"
#include "libnet/tcp_server.hpp"

#include <gtest/gtest.h>

#include <boost/asio.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

uint16_t reserveLocalPort()
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(
        io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
    return acceptor.local_endpoint().port();
}

template <typename Predicate>
bool waitUntil(Predicate&& predicate, std::chrono::milliseconds timeout = 2s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(10ms);
    }
    return predicate();
}

} // namespace

using namespace grid::net;

TEST(TcpTransportTest, ClientAndServerExchangeMessages)
{
    const auto port = reserveLocalPort();

    std::mutex server_mutex;
    std::vector<Message> server_messages;
    std::atomic<int> connect_count{0};
    std::atomic<int> disconnect_count{0};

    TcpServer server(
        port,
        [&](std::shared_ptr<Session>, Message msg) {
            std::lock_guard<std::mutex> lock(server_mutex);
            server_messages.push_back(std::move(msg));
        },
        [&](std::shared_ptr<Session>) { ++connect_count; },
        [&](std::shared_ptr<Session>) { ++disconnect_count; });
    server.start();

    std::mutex client_mutex;
    std::vector<Message> client_messages;
    TcpClient client("127.0.0.1", port, [&](Message msg) {
        std::lock_guard<std::mutex> lock(client_mutex);
        client_messages.push_back(std::move(msg));
    });
    client.start();

    ASSERT_TRUE(waitUntil([&] {
        return connect_count.load() == 1 && server.clientCount() == 1 && client.isConnected();
    }));

    const Message from_client(MessageType::InputEvent, {1, 2, 3});
    client.send(from_client);

    ASSERT_TRUE(waitUntil([&] {
        std::lock_guard<std::mutex> lock(server_mutex);
        return server_messages.size() == 1;
    }));

    {
        std::lock_guard<std::mutex> lock(server_mutex);
        ASSERT_EQ(server_messages.size(), 1u);
        EXPECT_EQ(server_messages.front().type(), MessageType::InputEvent);
        EXPECT_EQ(server_messages.front().payload(), from_client.payload());
    }

    const Message from_server(MessageType::Control, {9, 8});
    server.broadcast(from_server);

    ASSERT_TRUE(waitUntil([&] {
        std::lock_guard<std::mutex> lock(client_mutex);
        return client_messages.size() == 1;
    }));

    {
        std::lock_guard<std::mutex> lock(client_mutex);
        ASSERT_EQ(client_messages.size(), 1u);
        EXPECT_EQ(client_messages.front().type(), MessageType::Control);
        EXPECT_EQ(client_messages.front().payload(), from_server.payload());
    }

    client.stop();

    EXPECT_TRUE(waitUntil([&] {
        return disconnect_count.load() == 1 && server.clientCount() == 0;
    }));

    server.stop();
}

TEST(NetworkBridgeTest, BridgesClientInputAndServerState)
{
    const auto port = reserveLocalPort();

    std::mutex server_mutex;
    std::vector<Message> server_messages;
    NetworkBridge server_bridge;
    server_bridge.hostServer(port, [&](Message msg) {
        std::lock_guard<std::mutex> lock(server_mutex);
        server_messages.push_back(std::move(msg));
    });

    std::mutex client_mutex;
    std::vector<Message> client_messages;
    NetworkBridge client_bridge;
    client_bridge.connectToServer("127.0.0.1", port, [&](Message msg) {
        std::lock_guard<std::mutex> lock(client_mutex);
        client_messages.push_back(std::move(msg));
    });

    ASSERT_TRUE(waitUntil([&] {
        return server_bridge.isServer()
            && server_bridge.isRunning()
            && client_bridge.isRunning()
            && server_bridge.clientCount() == 1;
    }));

    const auto input = serializeKeyEvent(42, 1, 3);
    client_bridge.sendToServer(input);

    ASSERT_TRUE(waitUntil([&] {
        std::lock_guard<std::mutex> lock(server_mutex);
        return server_messages.size() == 1;
    }));

    {
        std::lock_guard<std::mutex> lock(server_mutex);
        ASSERT_EQ(server_messages.size(), 1u);
        const auto decoded = deserializeKeyEvent(server_messages.front());
        EXPECT_EQ(server_messages.front().type(), MessageType::InputEvent);
        EXPECT_EQ(decoded.key, 42);
        EXPECT_EQ(decoded.action, 1);
        EXPECT_EQ(decoded.modifiers, 3);
    }

    const std::vector<AgentSnapshot> snapshots = {
        {"soldier_7", {4.0, 2.0, -1.0}, 75.0, 0.5, 2, 1, false, "driver_1"},
    };
    server_bridge.broadcastAgentStates(snapshots);

    ASSERT_TRUE(waitUntil([&] {
        std::lock_guard<std::mutex> lock(client_mutex);
        return client_messages.size() == 1;
    }));

    {
        std::lock_guard<std::mutex> lock(client_mutex);
        ASSERT_EQ(client_messages.size(), 1u);
        EXPECT_EQ(client_messages.front().type(), MessageType::AgentState);

        const auto decoded = deserializeAgentStates(client_messages.front());
        ASSERT_EQ(decoded.size(), 1u);
        EXPECT_EQ(decoded.front().name, "soldier_7");
        EXPECT_DOUBLE_EQ(decoded.front().position[0], 4.0);
        EXPECT_DOUBLE_EQ(decoded.front().position[1], 2.0);
        EXPECT_DOUBLE_EQ(decoded.front().position[2], -1.0);
        EXPECT_EQ(decoded.front().driverName, "driver_1");
    }

    client_bridge.stop();
    server_bridge.stop();

    EXPECT_FALSE(client_bridge.isRunning());
    EXPECT_FALSE(server_bridge.isRunning());
}

TEST(NetworkBridgeTest, DefaultStateNoOpMethodsAndSessionCallbacks)
{
    NetworkBridge idle_bridge;
    EXPECT_FALSE(idle_bridge.isServer());
    EXPECT_FALSE(idle_bridge.isRunning());
    EXPECT_EQ(idle_bridge.clientCount(), 0u);

    idle_bridge.broadcastAgentStates({});
    idle_bridge.broadcastMessage(Message(MessageType::Control, {1}));
    idle_bridge.sendToServer(Message(MessageType::InputEvent, {2}));
    idle_bridge.stop();

    const auto port = reserveLocalPort();
    std::atomic<int> connect_count{0};
    std::atomic<int> disconnect_count{0};
    std::atomic<int> message_count{0};

    NetworkBridge server_bridge;
    server_bridge.hostServer(
        port,
        [&](std::shared_ptr<Session>, Message) { ++message_count; },
        [&](std::shared_ptr<Session>) { ++connect_count; },
        [&](std::shared_ptr<Session>) { ++disconnect_count; });

    NetworkBridge client_bridge;
    client_bridge.connectToServer("127.0.0.1", port, [](Message) {});

    ASSERT_TRUE(waitUntil([&] { return connect_count.load() == 1 && server_bridge.clientCount() == 1; }));

    client_bridge.sendToServer(Message(MessageType::InputEvent, {7, 8}));
    ASSERT_TRUE(waitUntil([&] { return message_count.load() == 1; }));

    client_bridge.stop();
    EXPECT_TRUE(waitUntil([&] { return disconnect_count.load() == 1; }));

    server_bridge.stop();
}
