#include "libsim/base_agent.hpp"
#include "libsim/base_engine.hpp"

#include <gtest/gtest.h>

#include <thread>
#include <type_traits>

namespace {

template <typename NumberType, size_t Dimensions>
grid::libsim::VectX<NumberType, Dimensions> makeCoord(NumberType base = NumberType{})
{
    grid::libsim::VectX<NumberType, Dimensions> coord{};
    for (size_t i = 0; i < Dimensions; ++i) {
        coord.values[i] = static_cast<NumberType>(base + static_cast<NumberType>(i));
    }
    return coord;
}

template <typename NumberType, size_t Dimensions>
grid::libsim::RangeXD<NumberType, Dimensions> makeRange(NumberType min, NumberType max)
{
    grid::libsim::RangeXD<NumberType, Dimensions> range{};
    for (size_t i = 0; i < Dimensions; ++i) {
        range.ranges[i] = {min, max};
    }
    return range;
}

template <typename NumberType, size_t Dimensions>
void exerciseEngineAndAgent()
{
    using namespace grid::libsim;
    using Engine = BaseEngine<NumberType, Dimensions>;
    using Agent = BaseAgent<NumberType, Dimensions>;

    Engine engine;
    auto location = makeCoord<NumberType, Dimensions>(static_cast<NumberType>(1));
    auto agent = std::make_shared<Agent>(engine, location, "templated-agent");

    engine.addAgent(agent);
    EXPECT_EQ(engine.getState(), State::STOPPED);
    EXPECT_EQ(engine.getAllAgents().size(), 1u);
    EXPECT_EQ(engine.getAgentsInRange(makeRange<NumberType, Dimensions>(
                  static_cast<NumberType>(0), static_cast<NumberType>(10))).size(), 1u);

    agent->start();
    agent->update(DeltaType(0.016f));
    agent->communicate(Senses::Sight, "visible");
    agent->on_destroy();

    auto snapshot = engine.snapshotAgentPositions();
    ASSERT_EQ(snapshot.size(), 1u);
    EXPECT_EQ(snapshot.at("templated-agent"), location);

    std::thread thread = engine.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    engine.stop();
    thread.join();

    engine.removeAgent(agent);
    EXPECT_TRUE(engine.getAllAgents().empty());
}

} // namespace

TEST(TemplateInstantiationSmokeTest, CoversInt2D)
{
    exerciseEngineAndAgent<int, 2>();
}

TEST(TemplateInstantiationSmokeTest, CoversDouble2D)
{
    exerciseEngineAndAgent<double, 2>();
}

TEST(TemplateInstantiationSmokeTest, CoversInt3D)
{
    exerciseEngineAndAgent<int, 3>();
}

TEST(TemplateInstantiationSmokeTest, CoversDouble3D)
{
    exerciseEngineAndAgent<double, 3>();
}
