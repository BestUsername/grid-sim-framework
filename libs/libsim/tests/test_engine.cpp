#include "gtest/gtest.h"
#include "libsim/base_engine.hpp"
#include "libsim/base_agent.hpp"
#include "libsim/i_environment.hpp"
#include "libevent/event_component.hpp"

#include <thread>  // Include this for std::thread
#include <chrono> // Add this include for std::chrono

// Add the include path for gmock
#include <gmock/gmock.h>

using ::testing::_; // Matcher for any value

using namespace grid::libsim;

using NUMBER_TYPE = int;
constexpr int NUM_DIMENSIONS = 2;
using COORD = VectX<NUMBER_TYPE, NUM_DIMENSIONS>;
using BASE_AGENT = BaseAgent<NUMBER_TYPE, NUM_DIMENSIONS>;
using I_AGENT = IAgent<NUMBER_TYPE, NUM_DIMENSIONS>;

class MockAgent : public BASE_AGENT {
public:
    MockAgent(IEnvironment<NUMBER_TYPE, NUM_DIMENSIONS>& env, const COORD& location, const std::string& name="John Doe"): BASE_AGENT(env, location, name){};
    MOCK_METHOD1(update, void(DeltaType));
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(on_destroy, void());
};

class EngineTest : public ::testing::Test {
protected:
    BaseEngine<NUMBER_TYPE, NUM_DIMENSIONS> engine;
    std::shared_ptr<MockAgent> agent;
    const COORD location{0,0};

    void SetUp() override {
        agent = std::make_shared<MockAgent>(engine, location);
        engine.addAgent(agent);
    }

    void TearDown() override {
        engine.removeAgent(agent);
    }
};

TEST_F(EngineTest, TestRunStep) {
    engine.setState(State::RUNNING);
    EXPECT_CALL(*agent, update(_)).Times(1);
    engine.run_step(DeltaType(100));
}

TEST_F(EngineTest, TestRun) {
    EXPECT_CALL(*agent, start()).Times(1);
    EXPECT_CALL(*agent, update(_)).Times(::testing::AtLeast(1));
    EXPECT_CALL(*agent, on_destroy()).Times(1);
    engine.setState(State::RUNNING);
    std::thread engineThread{ engine.run()};
    std::this_thread::sleep_for(std::chrono::seconds(1));
    engine.stop();
    engineThread.join();
}

TEST_F(EngineTest, TestAddRemoveAgent) {
    engine.setState(State::RUNNING);
    auto agent2 = std::make_shared<MockAgent>(engine, location);
    engine.addAgent(agent2);
    EXPECT_CALL(*agent, update(_)).Times(1);
    EXPECT_CALL(*agent2, update(_)).Times(1);
    engine.run_step(std::chrono::duration<double>(0.1));
    engine.removeAgent(agent2);
    EXPECT_CALL(*agent, update(_)).Times(1);
    EXPECT_CALL(*agent2, update(_)).Times(0);
    engine.run_step(std::chrono::duration<double>(0.1));
}

TEST_F(EngineTest, TestAddAgent) {
    auto agent2 = std::make_shared<MockAgent>(engine, location);
    engine.addAgent(agent2);
    auto agents = engine.getAllAgents();
    ASSERT_EQ(2, agents.size());
    EXPECT_EQ(agent, agents[0]);
    EXPECT_EQ(agent2, agents[1]);
}

TEST_F(EngineTest, TestRemoveAgent) {
    engine.removeAgent(agent);
    auto agents = engine.getAllAgents();
    ASSERT_EQ(0, agents.size());
}

TEST_F(EngineTest, TestGetState) {
    State state = engine.getState();
    EXPECT_EQ(State::STOPPED, state);
}

TEST_F(EngineTest, TestSetState) {
    engine.setState(State::RUNNING);
    State state = engine.getState();
    EXPECT_EQ(State::RUNNING, state);
}

TEST_F(EngineTest, TestStop) {
    engine.setState(State::RUNNING);
    engine.stop();
    State state = engine.getState();
    EXPECT_EQ(State::STOPPED, state);
}

TEST_F(EngineTest, TestPauseStopsUpdates) {
    EXPECT_CALL(*agent, start()).Times(1);
    EXPECT_CALL(*agent, on_destroy()).Times(1);
    // Agent should receive some updates while running, then none while paused
    EXPECT_CALL(*agent, update(_)).Times(::testing::AtLeast(1));

    std::thread engineThread{engine.run()};
    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Pause the engine — agent updates should stop
    engine.setState(State::PAUSED);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify the engine is still alive but paused
    EXPECT_EQ(State::PAUSED, engine.getState());

    engine.stop();
    engineThread.join();
}

// ===========================================================================
// Event integration tests
// ===========================================================================

using grid::libevent::Event;
using grid::libevent::TEvent;
using grid::libevent::EventKey;
using grid::libevent::event_cast;

/// An agent that records events delivered via on_event().
class EventRecordingAgent : public BASE_AGENT {
public:
    EventRecordingAgent(IEnvironment<NUMBER_TYPE, NUM_DIMENSIONS>& env,
                        const COORD& location,
                        const std::string& name = "Recorder")
        : BASE_AGENT(env, location, name) {}

    void on_event(Event* incoming) override {
        m_receivedKeys.push_back(incoming->GetKey());
        auto* typed = event_cast<int>(incoming);
        if (typed) {
            m_lastValue = typed->GetObject();
        }
    }

    void start() override {}
    void on_destroy() override {}

    std::vector<EventKey> m_receivedKeys;
    int m_lastValue = 0;
};

class EngineEventTest : public ::testing::Test {
protected:
    BaseEngine<NUMBER_TYPE, NUM_DIMENSIONS> engine;

    void TearDown() override {
    }
};

TEST_F(EngineEventTest, EngineForwardsEventAPI) {
    // BaseEngine exposes Subscribe/SendEvent/PollEvents without being an EventComponent
    engine.Subscribe("test-key");
    EXPECT_TRUE(engine.IsSubscribed("test-key"));
    engine.Unsubscribe("test-key");
    EXPECT_FALSE(engine.IsSubscribed("test-key"));
}

TEST_F(EngineEventTest, PollEventsDispatchesToAgents) {
    auto agent = std::make_shared<EventRecordingAgent>(engine, COORD{0, 0});
    engine.addAgent(agent);

    // Subscribe the engine to an event key
    engine.Subscribe("test");

    // Send an event — it goes into the engine's queue
    int value = 42;
    engine.SendEvent(TEvent<int>("test", value));

    // No events dispatched yet
    EXPECT_TRUE(agent->m_receivedKeys.empty());

    // Poll — engine drains its queue, calling ProcessEvent which dispatches to agents
    size_t processed = engine.PollEvents();

    EXPECT_EQ(processed, 1u);
    ASSERT_EQ(agent->m_receivedKeys.size(), 1u);
    EXPECT_EQ(agent->m_receivedKeys[0], "test");
    EXPECT_EQ(agent->m_lastValue, 42);

    engine.Unsubscribe("test");
}

TEST_F(EngineEventTest, RunStepPollsEvents) {
    auto agent = std::make_shared<EventRecordingAgent>(engine, COORD{0, 0});
    engine.addAgent(agent);

    engine.Subscribe("step-test");

    int value = 99;
    engine.SendEvent(TEvent<int>("step-test", value));

    // run_step should poll events before updating agents
    engine.run_step(DeltaType(0.016));

    ASSERT_EQ(agent->m_receivedKeys.size(), 1u);
    EXPECT_EQ(agent->m_receivedKeys[0], "step-test");
    EXPECT_EQ(agent->m_lastValue, 99);

    engine.Unsubscribe("step-test");
}

TEST_F(EngineEventTest, MultipleAgentsReceiveEvent) {
    auto a1 = std::make_shared<EventRecordingAgent>(engine, COORD{0, 0}, "A1");
    auto a2 = std::make_shared<EventRecordingAgent>(engine, COORD{1, 1}, "A2");
    engine.addAgent(a1);
    engine.addAgent(a2);

    engine.Subscribe("broadcast");

    int value = 7;
    engine.SendEvent(TEvent<int>("broadcast", value));
    engine.PollEvents();

    ASSERT_EQ(a1->m_receivedKeys.size(), 1u);
    ASSERT_EQ(a2->m_receivedKeys.size(), 1u);
    EXPECT_EQ(a1->m_lastValue, 7);
    EXPECT_EQ(a2->m_lastValue, 7);

    engine.Unsubscribe("broadcast");
}

TEST_F(EngineEventTest, NoEventsNoDispatch) {
    auto agent = std::make_shared<EventRecordingAgent>(engine, COORD{0, 0});
    engine.addAgent(agent);

    size_t processed = engine.PollEvents();

    EXPECT_EQ(processed, 0u);
    EXPECT_TRUE(agent->m_receivedKeys.empty());
}

TEST_F(EngineEventTest, PlainEventDispatchesToAgents) {
    auto agent = std::make_shared<EventRecordingAgent>(engine, COORD{0, 0});
    engine.addAgent(agent);

    engine.Subscribe("signal");
    engine.SendEvent(Event("signal"));
    engine.PollEvents();

    ASSERT_EQ(agent->m_receivedKeys.size(), 1u);
    EXPECT_EQ(agent->m_receivedKeys[0], "signal");

    engine.Unsubscribe("signal");
}

TEST_F(EngineEventTest, EventsDuringRunLoop) {
    auto agent = std::make_shared<EventRecordingAgent>(engine, COORD{0, 0});
    engine.addAgent(agent);

    engine.Subscribe("live-event");

    // Start the engine
    std::thread engineThread{engine.run()};

    // Give the engine a moment to start running
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send an event while the engine is running
    int value = 123;
    engine.SendEvent(TEvent<int>("live-event", value));

    // Give it time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    engine.stop();
    engineThread.join();

    // The agent should have received the event during the run loop
    ASSERT_GE(agent->m_receivedKeys.size(), 1u);
    EXPECT_EQ(agent->m_receivedKeys[0], "live-event");
    EXPECT_EQ(agent->m_lastValue, 123);

    engine.Unsubscribe("live-event");
}

TEST_F(EngineEventTest, AgentCanBroadcastViaEnvironment) {
    auto agent = std::make_shared<EventRecordingAgent>(engine, COORD{0, 0});
    engine.addAgent(agent);

    engine.Subscribe("env-event");

    // Use the IEnvironment interface to broadcast an event
    IEnvironment<NUMBER_TYPE, NUM_DIMENSIONS>& env = engine;
    int value = 55;
    env.broadcastEvent(TEvent<int>("env-event", value));
    engine.PollEvents();

    ASSERT_EQ(agent->m_receivedKeys.size(), 1u);
    EXPECT_EQ(agent->m_receivedKeys[0], "env-event");
    EXPECT_EQ(agent->m_lastValue, 55);

    engine.Unsubscribe("env-event");
}

// ===========================================================================
// snapshotAgentPositions tests
// ===========================================================================

TEST_F(EngineTest, SnapshotEmptyEngine) {
    engine.removeAgent(agent);
    auto snap = engine.snapshotAgentPositions();
    EXPECT_TRUE(snap.empty());
}

TEST_F(EngineTest, SnapshotReturnsCorrectPositions) {
    auto snap = engine.snapshotAgentPositions();
    ASSERT_EQ(snap.size(), 1u);
    auto it = snap.find("John Doe");
    ASSERT_NE(it, snap.end());
    EXPECT_EQ(it->second[0], 0);
    EXPECT_EQ(it->second[1], 0);
}

TEST_F(EngineTest, SnapshotMultipleAgents) {
    COORD loc2{5, 3};
    auto agent2 = std::make_shared<MockAgent>(engine, loc2, "Agent2");
    engine.addAgent(agent2);

    auto snap = engine.snapshotAgentPositions();
    ASSERT_EQ(snap.size(), 2u);
    EXPECT_EQ(snap["John Doe"][0], 0);
    EXPECT_EQ(snap["Agent2"][0], 5);
    EXPECT_EQ(snap["Agent2"][1], 3);

    engine.removeAgent(agent2);
}

TEST_F(EngineTest, SnapshotReflectsPositionChange) {
    agent->set_location(COORD{10, 20});
    auto snap = engine.snapshotAgentPositions();
    EXPECT_EQ(snap["John Doe"][0], 10);
    EXPECT_EQ(snap["John Doe"][1], 20);
}

TEST_F(EngineTest, WithAgentsLockExecutesCallable) {
    int result = engine.withAgentsLock([&]() {
        return static_cast<int>(engine.getAllAgents().size());
    });
    EXPECT_EQ(result, 1);
}
