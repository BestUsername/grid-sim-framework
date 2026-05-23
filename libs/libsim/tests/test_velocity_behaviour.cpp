#include <gtest/gtest.h>
#include "libsim/base_agent.hpp"
#include "libsim/base_engine.hpp"
#include "libsim/velocity_behaviour.hpp"

using namespace grid::libsim;

using NUMBER_TYPE = double;
constexpr size_t NUM_DIMENSIONS = 2;
using COORD = VectX<NUMBER_TYPE, NUM_DIMENSIONS>;

/**
 * @brief Test agent that exposes velocity control for testing.
 */
class TestAgent : public BaseAgent<NUMBER_TYPE, NUM_DIMENSIONS> {
public:
    TestAgent(IEnvironment<NUMBER_TYPE, NUM_DIMENSIONS>& env,
              const COORD& location,
              const COORD& velocity = COORD{0.0, 0.0},
              const std::string& name = "TestAgent")
        : BaseAgent(env, location, name)
    {
        addBehaviour(std::make_unique<VelocityBehaviour<NUMBER_TYPE, NUM_DIMENSIONS>>(*this, velocity));
    }
};

class VelocityBehaviourTest : public ::testing::Test {
protected:
    BaseEngine<NUMBER_TYPE, NUM_DIMENSIONS> engine;
    COORD origin{0.0, 0.0};
};

TEST_F(VelocityBehaviourTest, ZeroVelocityDoesNotMove) {
    auto agent = std::make_shared<TestAgent>(engine, origin, COORD{0.0, 0.0});
    agent->update(std::chrono::duration<double>(1.0));
    EXPECT_DOUBLE_EQ(agent->location().values[0], 0.0);
    EXPECT_DOUBLE_EQ(agent->location().values[1], 0.0);
}

TEST_F(VelocityBehaviourTest, MovesWithVelocity) {
    auto agent = std::make_shared<TestAgent>(engine, origin, COORD{10.0, 5.0});
    agent->update(std::chrono::duration<double>(1.0));
    EXPECT_DOUBLE_EQ(agent->location().values[0], 10.0);
    EXPECT_DOUBLE_EQ(agent->location().values[1], 5.0);
}

TEST_F(VelocityBehaviourTest, ScalesWithDelta) {
    auto agent = std::make_shared<TestAgent>(engine, origin, COORD{10.0, 10.0});
    // Update with delta of 0.5 seconds — should move half the velocity
    agent->update(std::chrono::duration<double>(0.5));
    EXPECT_DOUBLE_EQ(agent->location().values[0], 5.0);
    EXPECT_DOUBLE_EQ(agent->location().values[1], 5.0);
}

TEST_F(VelocityBehaviourTest, AccumulatesOverMultipleUpdates) {
    auto agent = std::make_shared<TestAgent>(engine, origin, COORD{1.0, 2.0});
    agent->update(std::chrono::duration<double>(1.0));
    agent->update(std::chrono::duration<double>(1.0));
    agent->update(std::chrono::duration<double>(1.0));

    EXPECT_DOUBLE_EQ(agent->location().values[0], 3.0);
    EXPECT_DOUBLE_EQ(agent->location().values[1], 6.0);
}

TEST_F(VelocityBehaviourTest, NegativeVelocity) {
    COORD start{10.0, 10.0};
    auto agent = std::make_shared<TestAgent>(engine, start, COORD{-5.0, -3.0});
    agent->update(std::chrono::duration<double>(1.0));
    EXPECT_DOUBLE_EQ(agent->location().values[0], 5.0);
    EXPECT_DOUBLE_EQ(agent->location().values[1], 7.0);
}
