#include "gtest/gtest.h"
#include "libsim/sense_event.hpp"
#include "libsim/base_engine.hpp"
#include "libsim/base_agent.hpp"
#include "libsim/types.hpp"

#include <cmath>
#include <string>
#include <vector>

using namespace grid::libsim;

using NUMBER_TYPE = int;
constexpr size_t NUM_DIMENSIONS = 2;
using COORD = VectX<NUMBER_TYPE, NUM_DIMENSIONS>;
using SenseEvt = SenseEvent<NUMBER_TYPE, NUM_DIMENSIONS>;
using AudioEvt = AudioEvent<NUMBER_TYPE, NUM_DIMENSIONS>;
using Engine   = BaseEngine<NUMBER_TYPE, NUM_DIMENSIONS>;
using Agent    = BaseAgent<NUMBER_TYPE, NUM_DIMENSIONS>;

// =========================================================================
// SenseEvent unit tests
// =========================================================================

TEST(SenseEventTest, ConstructionAndAccessors) {
    COORD origin{10, 20};
    AudioEvt evt("Alice", origin, 1.0f, 50.0f, "Hello!");

    EXPECT_EQ(evt.source(), "Alice");
    EXPECT_EQ(evt.origin(), origin);
    EXPECT_EQ(evt.sense(), Senses::Hearing);
    EXPECT_FLOAT_EQ(evt.intensity(), 1.0f);
    EXPECT_FLOAT_EQ(evt.range(), 50.0f);
    EXPECT_EQ(evt.message(), "Hello!");
    EXPECT_EQ(evt.GetKey(), "sense.audio");
}

TEST(SenseEventTest, ContainsPointInsideRange) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 10.0f, "msg");

    EXPECT_TRUE(evt.contains(COORD{0, 0}));   // at origin
    EXPECT_TRUE(evt.contains(COORD{7, 0}));   // inside
    EXPECT_TRUE(evt.contains(COORD{0, 9}));   // inside
    EXPECT_TRUE(evt.contains(COORD{10, 0}));  // exactly on boundary
}

TEST(SenseEventTest, DoesNotContainPointOutsideRange) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 10.0f, "msg");

    EXPECT_FALSE(evt.contains(COORD{11, 0}));  // just outside
    EXPECT_FALSE(evt.contains(COORD{8, 8}));   // diagonal ~11.3
    EXPECT_FALSE(evt.contains(COORD{50, 50})); // far away
}

TEST(SenseEventTest, LinearAttenuationAtOrigin) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 100.0f, "msg");

    EXPECT_FLOAT_EQ(evt.attenuate(0.0f), 1.0f);  // full at origin
}

TEST(SenseEventTest, LinearAttenuationAtRange) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 100.0f, "msg");

    EXPECT_FLOAT_EQ(evt.attenuate(100.0f), 0.0f); // zero at max range
}

TEST(SenseEventTest, LinearAttenuationMidRange) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 100.0f, "msg");

    EXPECT_FLOAT_EQ(evt.attenuate(50.0f), 0.5f);  // half at midpoint
    EXPECT_FLOAT_EQ(evt.attenuate(25.0f), 0.75f);
    EXPECT_FLOAT_EQ(evt.attenuate(75.0f), 0.25f);
}

TEST(SenseEventTest, AttenuationBeyondRange) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 10.0f, "msg");

    EXPECT_FLOAT_EQ(evt.attenuate(15.0f), 0.0f);  // beyond range
    EXPECT_FLOAT_EQ(evt.attenuate(100.0f), 0.0f);
}

TEST(SenseEventTest, AttenuationScalesWithIntensity) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 2.0f, 100.0f, "msg");

    EXPECT_FLOAT_EQ(evt.attenuate(0.0f), 2.0f);   // double intensity at origin
    EXPECT_FLOAT_EQ(evt.attenuate(50.0f), 1.0f);   // half of 2.0
}

TEST(SenseEventTest, DistanceToComputation) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 100.0f, "msg");

    EXPECT_FLOAT_EQ(evt.distanceTo(COORD{3, 4}), 5.0f);   // 3-4-5 triangle
    EXPECT_FLOAT_EQ(evt.distanceTo(COORD{0, 0}), 0.0f);
}

TEST(SenseEventTest, DistanceWithOffset) {
    COORD origin{10, 10};
    AudioEvt evt("A", origin, 1.0f, 100.0f, "msg");

    EXPECT_FLOAT_EQ(evt.distanceTo(COORD{13, 14}), 5.0f);  // still 3-4-5
}

TEST(SenseEventTest, ClonePreservesType) {
    COORD origin{5, 5};
    AudioEvt evt("A", origin, 1.0f, 50.0f, "test");

    auto cloned = evt.clone();
    auto* typed = dynamic_cast<AudioEvt*>(cloned.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->source(), "A");
    EXPECT_EQ(typed->message(), "test");
    EXPECT_FLOAT_EQ(typed->intensity(), 1.0f);
}

TEST(SenseEventTest, PerceivedIntensityDefaultsToZero) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 50.0f, "msg");
    EXPECT_FLOAT_EQ(evt.perceivedIntensity(), 0.0f);
}

TEST(SenseEventTest, SetPerceivedIntensity) {
    COORD origin{0, 0};
    AudioEvt evt("A", origin, 1.0f, 50.0f, "msg");
    evt.setPerceivedIntensity(0.42f);
    EXPECT_FLOAT_EQ(evt.perceivedIntensity(), 0.42f);
}

// =========================================================================
// Engine emitSenseEvent integration tests
// =========================================================================

/// A test agent that records every on_event call.
class RecordingAgent : public Agent {
public:
    using Agent::Agent;

    struct ReceivedEvent {
        std::string key;
        float perceivedIntensity;
    };
    std::vector<ReceivedEvent> received;

    void on_event(grid::libevent::Event* event) override {
        auto* se = dynamic_cast<SenseEvt*>(event);
        if (se) {
            received.push_back({se->GetKey(), se->perceivedIntensity()});
        }
    }

    // Silence the start/on_destroy communicate calls for cleaner tests
    void start() override {}
    void on_destroy() override {}
};

TEST(EmitSenseEventTest, AgentInRangeReceivesEvent) {
    Engine engine;
    COORD origin{10, 10};
    COORD agentPos{13, 14};  // distance 5 from origin

    auto agent = std::make_shared<RecordingAgent>(engine, agentPos, "Listener");
    engine.addAgent(agent);

    AudioEvt audio("Speaker", origin, 1.0f, 50.0f, "Hello!");
    engine.emitSenseEvent(audio);

    ASSERT_EQ(agent->received.size(), 1u);
    EXPECT_EQ(agent->received[0].key, "sense.audio");
    // distance = 5, range = 50 → perceived = 1.0 * (1 - 5/50) = 0.9
    EXPECT_FLOAT_EQ(agent->received[0].perceivedIntensity, 0.9f);
}

TEST(EmitSenseEventTest, AgentOutOfRangeDoesNotReceive) {
    Engine engine;
    COORD origin{0, 0};
    COORD farAway{100, 100};  // distance ~141

    auto agent = std::make_shared<RecordingAgent>(engine, farAway, "FarListener");
    engine.addAgent(agent);

    AudioEvt audio("Speaker", origin, 1.0f, 50.0f, "Hello!");
    engine.emitSenseEvent(audio);

    EXPECT_TRUE(agent->received.empty());
}

TEST(EmitSenseEventTest, MultipleAgentsAtDifferentDistances) {
    Engine engine;
    COORD origin{0, 0};

    auto close = std::make_shared<RecordingAgent>(engine, COORD{5, 0}, "Close");
    auto mid   = std::make_shared<RecordingAgent>(engine, COORD{25, 0}, "Mid");
    auto far   = std::make_shared<RecordingAgent>(engine, COORD{45, 0}, "Far");
    auto out   = std::make_shared<RecordingAgent>(engine, COORD{60, 0}, "Out");

    engine.addAgent(close);
    engine.addAgent(mid);
    engine.addAgent(far);
    engine.addAgent(out);

    AudioEvt audio("Speaker", origin, 1.0f, 50.0f, "Hello!");
    engine.emitSenseEvent(audio);

    // close: dist=5, perceived = 1.0 * (1 - 5/50) = 0.9
    ASSERT_EQ(close->received.size(), 1u);
    EXPECT_FLOAT_EQ(close->received[0].perceivedIntensity, 0.9f);

    // mid: dist=25, perceived = 1.0 * (1 - 25/50) = 0.5
    ASSERT_EQ(mid->received.size(), 1u);
    EXPECT_FLOAT_EQ(mid->received[0].perceivedIntensity, 0.5f);

    // far: dist=45, perceived = 1.0 * (1 - 45/50) = 0.1
    ASSERT_EQ(far->received.size(), 1u);
    EXPECT_FLOAT_EQ(far->received[0].perceivedIntensity, 0.1f);

    // out: dist=60, beyond range → no event
    EXPECT_TRUE(out->received.empty());
}

TEST(EmitSenseEventTest, AgentAtOriginGetsFullIntensity) {
    Engine engine;
    COORD origin{20, 20};

    auto agent = std::make_shared<RecordingAgent>(engine, origin, "AtOrigin");
    engine.addAgent(agent);

    AudioEvt audio("Speaker", origin, 1.0f, 50.0f, "Hello!");
    engine.emitSenseEvent(audio);

    ASSERT_EQ(agent->received.size(), 1u);
    EXPECT_FLOAT_EQ(agent->received[0].perceivedIntensity, 1.0f);
}

TEST(EmitSenseEventTest, EmitLogsToGameLog) {
    Engine engine;

    AudioEvt audio("Speaker", COORD{5, 10}, 1.0f, 50.0f, "Hello!");
    engine.emitSenseEvent(audio);

    ASSERT_EQ(engine.getGameLog().size(), 1u);
    auto entries = engine.getGameLog().getEntries();
    EXPECT_EQ(entries[0].source, "Speaker");
    EXPECT_EQ(entries[0].message, "Hello!");
    EXPECT_EQ(entries[0].sense, Senses::Hearing);
}

TEST(EmitSenseEventTest, EmitWithNoAgentsDoesNotCrash) {
    Engine engine;

    AudioEvt audio("Speaker", COORD{0, 0}, 1.0f, 50.0f, "Hello!");
    engine.emitSenseEvent(audio);

    EXPECT_EQ(engine.getGameLog().size(), 1u);
}

// =========================================================================
// communicate() → AudioEvent integration
// =========================================================================

TEST(CommunicateAudioTest, CommunicateHearingEmitsAudioEvent) {
    Engine engine;
    COORD speakerPos{10, 10};
    COORD listenerPos{12, 16};  // distance = sqrt(4+36) = sqrt(40) ~ 6.32

    auto speaker  = std::make_shared<RecordingAgent>(engine, speakerPos, "Speaker");
    auto listener = std::make_shared<RecordingAgent>(engine, listenerPos, "Listener");
    engine.addAgent(speaker);
    engine.addAgent(listener);

    speaker->communicate(Senses::Hearing, "I live!");

    // Both speaker and listener are within default range (50)
    // Speaker is at distance 0 from itself
    ASSERT_EQ(speaker->received.size(), 1u);
    EXPECT_FLOAT_EQ(speaker->received[0].perceivedIntensity, 1.0f);

    // Listener at ~6.32
    ASSERT_EQ(listener->received.size(), 1u);
    float expectedDist = std::sqrt(4.0f + 36.0f);
    float expectedPerceived = 1.0f * (1.0f - expectedDist / 50.0f);
    EXPECT_NEAR(listener->received[0].perceivedIntensity, expectedPerceived, 0.001f);
}

TEST(CommunicateAudioTest, CommunicateNonHearingJustLogs) {
    Engine engine;
    auto agent = std::make_shared<RecordingAgent>(engine, COORD{5, 5}, "Seer");
    engine.addAgent(agent);

    // Sight doesn't emit a sense event (yet), just logs
    agent->communicate(Senses::Sight, "I see you");

    EXPECT_TRUE(agent->received.empty());
    ASSERT_EQ(engine.getGameLog().size(), 1u);
    auto entries = engine.getGameLog().getEntries();
    EXPECT_EQ(entries[0].sense, Senses::Sight);
    EXPECT_EQ(entries[0].message, "I see you");
}

// =========================================================================
// Custom SenseEvent subclass test (verifying extensibility)
// =========================================================================

/// Example: a visual event with a rectangular emission area.
class VisualEvent : public SenseEvt {
public:
    VisualEvent(const std::string& source,
                const COORD& origin,
                float intensity,
                const COORD& halfExtents,
                const std::string& message)
        : SenseEvt("sense.visual", source, origin,
                    Senses::Sight, intensity,
                    /*range (used for attenuation scale)=*/
                    static_cast<float>(std::max(halfExtents[0], halfExtents[1])),
                    message)
        , m_halfExtents(halfExtents)
    {}

    bool contains(const COORD& pos) const override {
        for (size_t i = 0; i < NUM_DIMENSIONS; ++i) {
            auto diff = std::abs(pos[i] - m_origin[i]);
            if (diff > m_halfExtents[i]) return false;
        }
        return true;
    }

    std::unique_ptr<grid::libevent::Event> clone() const override {
        return std::make_unique<VisualEvent>(*this);
    }

private:
    COORD m_halfExtents;
};

TEST(CustomSenseEventTest, RectangularEmissionShape) {
    Engine engine;

    // Agent inside the rectangle
    auto inside = std::make_shared<RecordingAgent>(engine, COORD{12, 10}, "Inside");
    // Agent outside the rectangle
    auto outside = std::make_shared<RecordingAgent>(engine, COORD{30, 10}, "Outside");
    engine.addAgent(inside);
    engine.addAgent(outside);

    VisualEvent vis("Watcher", COORD{10, 10}, 1.0f, COORD{5, 3}, "Flash!");
    engine.emitSenseEvent(vis);

    EXPECT_EQ(inside->received.size(), 1u);
    EXPECT_TRUE(outside->received.empty());
}
