#include "gtest/gtest.h"
#include "libsim/types.hpp"
#include "libsim/shapes.hpp"
#include "libsim/base_agent.hpp"
#include "libsim/base_engine.hpp"


// Add the include path for gmock
#include <gmock/gmock.h>

using ::testing::_; // Matcher for any value

using namespace grid::libsim;

using NUMBER_TYPE = int;
constexpr int NUM_DIMENSIONS = 2;
using COORD = VectX<NUMBER_TYPE, NUM_DIMENSIONS>;
using BASE_AGENT = BaseAgent<NUMBER_TYPE, NUM_DIMENSIONS>;

class MockEngine: public BaseEngine<NUMBER_TYPE, NUM_DIMENSIONS> {
public:
    MockEngine(): BaseEngine<NUMBER_TYPE, NUM_DIMENSIONS>(){};
    MOCK_METHOD1(addAgent, void(BASE_AGENT*));
    MOCK_METHOD1(removeAgent, void(BASE_AGENT*));
    MOCK_METHOD1(run_step, void(std::chrono::duration<double>));
    MOCK_METHOD0(run, void());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD1(getAgentsInRange, std::vector<BASE_AGENT*>(const RangeXD<NUMBER_TYPE, NUM_DIMENSIONS>));
};

TEST(BaseAgentTest, TestName) {
    MockEngine env;
    COORD location{0, 0};
    std::string name = "Agent1";
    BASE_AGENT agent(env, location, name);
    EXPECT_EQ(agent.name(), name);
}

TEST(BaseAgentTest, TestLocation) {
    MockEngine env;
    COORD location{5, 10};
    std::string name = "Agent2";
    BASE_AGENT agent(env, location, name);
    EXPECT_EQ(agent.location(), location);
}

TEST(BaseAgentTest, TestStart) {
    MockEngine env;
    COORD location{0, 0};
    std::string name = "Agent3";
    BASE_AGENT agent(env, location, name);
    agent.start();
    // start() calls communicate() which logs to the game log
    ASSERT_EQ(env.getGameLog().size(), 1u);
    auto entries = env.getGameLog().getEntries();
    EXPECT_EQ(entries[0].source, "Agent3");
    EXPECT_EQ(entries[0].message, "I live!");
    EXPECT_EQ(entries[0].sense, Senses::Hearing);
}

TEST(BaseAgentTest, TestUpdate) {
    MockEngine env;
    COORD location{0, 0};
    std::string name = "Agent4";
    BASE_AGENT agent(env, location, name);
    std::chrono::duration<double> delta(1.0);
    agent.update(delta);
    EXPECT_EQ(agent.location(), location); // Since velocity is (0, 0), location should remain the same
}

TEST(BaseAgentTest, TestCommunicate) {
    MockEngine env;
    COORD location{3, 7};
    std::string name = "Agent8";
    BASE_AGENT agent(env, location, name);
    agent.communicate(Senses::Hearing, "Hello!");
    ASSERT_EQ(env.getGameLog().size(), 1u);
    auto entries = env.getGameLog().getEntries();
    EXPECT_EQ(entries[0].source, "Agent8");
    EXPECT_EQ(entries[0].location, "(3, 7)");
    EXPECT_EQ(entries[0].sense, Senses::Hearing);
    EXPECT_EQ(entries[0].message, "Hello!");
}

TEST(BaseAgentTest, TestOperatorOutput) {
    MockEngine env;
    COORD location{0, 0};
    std::string name = "Agent5";
    BASE_AGENT agent(env, location, name);
    std::ostringstream oss;
    oss << agent;
    std::string output = oss.str();
    EXPECT_EQ(output, "Agent5(0, 0)");
}

TEST(BaseAgentTest, TestOnDestroy) {
    MockEngine env;
    COORD location{0, 0};
    std::string name = "Agent6";
    BASE_AGENT agent(env, location, name);
    agent.on_destroy();
    // on_destroy() calls communicate() which logs to the game log
    ASSERT_EQ(env.getGameLog().size(), 1u);
    auto entries = env.getGameLog().getEntries();
    EXPECT_EQ(entries[0].source, "Agent6");
    EXPECT_EQ(entries[0].message, "Aaaarrrrgh!");
    EXPECT_EQ(entries[0].sense, Senses::Hearing);
}

// ===========================================================================
// Behaviour system tests
// ===========================================================================

class CountingBehaviour : public IBehaviour {
public:
    int count = 0;
    void execute(DeltaType delta) override { ++count; }
};

class AnotherBehaviour : public IBehaviour {
public:
    int count = 0;
    void execute(DeltaType delta) override { count += 10; }
};

TEST(BehaviourSystemTest, AddAndExecuteBehaviour) {
    MockEngine env;
    COORD location{0, 0};
    BASE_AGENT agent(env, location, "BehaviourAgent");

    agent.addBehaviour(std::make_unique<CountingBehaviour>());
    agent.update(DeltaType(1.0));

    auto* cb = agent.getBehaviour<CountingBehaviour>();
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(cb->count, 1);
}

TEST(BehaviourSystemTest, RemoveBehaviour) {
    MockEngine env;
    COORD location{0, 0};
    BASE_AGENT agent(env, location, "BehaviourAgent");

    agent.addBehaviour(std::make_unique<CountingBehaviour>());
    auto* cb = agent.getBehaviour<CountingBehaviour>();
    ASSERT_NE(cb, nullptr);

    agent.removeBehaviour(cb);
    EXPECT_EQ(agent.getBehaviour<CountingBehaviour>(), nullptr);

    // Update should not crash with no behaviours
    agent.update(DeltaType(1.0));
}

TEST(BehaviourSystemTest, MultipleBehaviours) {
    MockEngine env;
    COORD location{0, 0};
    BASE_AGENT agent(env, location, "BehaviourAgent");

    agent.addBehaviour(std::make_unique<CountingBehaviour>());
    agent.addBehaviour(std::make_unique<AnotherBehaviour>());

    agent.update(DeltaType(1.0));

    auto* cb = agent.getBehaviour<CountingBehaviour>();
    auto* ab = agent.getBehaviour<AnotherBehaviour>();
    ASSERT_NE(cb, nullptr);
    ASSERT_NE(ab, nullptr);
    EXPECT_EQ(cb->count, 1);
    EXPECT_EQ(ab->count, 10);
}

TEST(BehaviourSystemTest, GetBehaviourReturnsNullWhenNotPresent) {
    MockEngine env;
    COORD location{0, 0};
    BASE_AGENT agent(env, location, "BehaviourAgent");

    EXPECT_EQ(agent.getBehaviour<CountingBehaviour>(), nullptr);
}
