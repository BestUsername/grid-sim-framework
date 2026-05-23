#include "gtest/gtest.h"
#include "libevent/event_component.hpp"
#include "libevent/event_bus.hpp"
#include "libevent/thread_event_component.hpp"

using namespace grid::libevent;

// ---------------------------------------------------------------------------
// A minimal non-threaded component for testing EventComponent directly.
// ---------------------------------------------------------------------------
class PollingComponent : public EventComponent {
public:
    explicit PollingComponent(EventBus& bus) : EventComponent(bus) {}
    PollingComponent() = default;

    void ProcessEvent(Event *incoming) override {
        m_receivedKeys.push_back(incoming->GetKey());

        auto *typed = event_cast<std::vector<int>>(incoming);
        if (typed) {
            m_lastData = typed->GetObject();
        }
    }

    void SubscribeTo(const EventKey& key) { Subscribe(key); }
    void UnsubscribeFrom(const EventKey& key) { Unsubscribe(key); }

    std::vector<EventKey> GetReceivedKeys() const { return m_receivedKeys; }
    std::vector<int> GetLastData() const { return m_lastData; }
    size_t GetReceivedCount() const { return m_receivedKeys.size(); }

private:
    std::vector<EventKey> m_receivedKeys;
    std::vector<int> m_lastData;
};

// ===========================================================================
// EventComponent (non-threaded) tests
// ===========================================================================

class EventComponentTest : public ::testing::Test {
protected:
    EventBus bus;
};

TEST_F(EventComponentTest, PollEventsProcessesPendingEvents) {
    PollingComponent comp(bus);
    comp.SubscribeTo("tick");

    std::vector<int> data = {1, 2, 3};
    comp.SendEvent(TEvent<std::vector<int>>("tick", data));
    comp.SendEvent(TEvent<std::vector<int>>("tick", data));

    EXPECT_EQ(comp.GetReceivedCount(), 0u);

    size_t processed = comp.PollEvents();

    EXPECT_EQ(processed, 2u);
    EXPECT_EQ(comp.GetReceivedCount(), 2u);
    EXPECT_EQ(comp.GetLastData(), data);
}

TEST_F(EventComponentTest, PollEventsReturnsZeroWhenEmpty) {
    PollingComponent comp(bus);
    EXPECT_EQ(comp.PollEvents(), 0u);
}

TEST_F(EventComponentTest, SubscribeAndSendWithoutThread) {
    PollingComponent sender(bus);
    PollingComponent receiver(bus);

    receiver.SubscribeTo("hello");

    std::vector<int> data = {42};
    sender.SendEvent(TEvent<std::vector<int>>("hello", data));

    EXPECT_EQ(receiver.GetReceivedCount(), 0u);

    size_t n = receiver.PollEvents();

    EXPECT_EQ(n, 1u);
    EXPECT_EQ(receiver.GetReceivedKeys()[0], "hello");
    EXPECT_EQ(receiver.GetLastData(), data);
}

TEST_F(EventComponentTest, MultipleSubscribersPollIndependently) {
    PollingComponent sender(bus);
    PollingComponent r1(bus), r2(bus);

    r1.SubscribeTo("evt");
    r2.SubscribeTo("evt");

    std::vector<int> data = {10};
    sender.SendEvent(TEvent<std::vector<int>>("evt", data));

    // Only r1 polls — r2 should still have a pending event.
    EXPECT_EQ(r1.PollEvents(), 1u);
    EXPECT_EQ(r1.GetReceivedCount(), 1u);
    EXPECT_EQ(r2.GetReceivedCount(), 0u);

    EXPECT_EQ(r2.PollEvents(), 1u);
    EXPECT_EQ(r2.GetReceivedCount(), 1u);
}

TEST_F(EventComponentTest, UnsubscribedComponentDoesNotReceive) {
    PollingComponent sender(bus);
    PollingComponent receiver(bus);

    receiver.SubscribeTo("msg");

    std::vector<int> first = {1};
    sender.SendEvent(TEvent<std::vector<int>>("msg", first));
    EXPECT_EQ(receiver.PollEvents(), 1u);

    receiver.UnsubscribeFrom("msg");

    std::vector<int> second = {2};
    sender.SendEvent(TEvent<std::vector<int>>("msg", second));
    EXPECT_EQ(receiver.PollEvents(), 0u);
    EXPECT_EQ(receiver.GetReceivedCount(), 1u);
}

TEST_F(EventComponentTest, PlainEventPreservesKeyViaPoll) {
    PollingComponent sender(bus);
    PollingComponent receiver(bus);

    receiver.SubscribeTo("signal");
    sender.SendEvent(Event("signal"));

    EXPECT_EQ(receiver.PollEvents(), 1u);
    EXPECT_EQ(receiver.GetReceivedKeys()[0], "signal");
}

TEST_F(EventComponentTest, TEventPayloadPreservedViaPoll) {
    PollingComponent sender(bus);
    PollingComponent receiver(bus);

    receiver.SubscribeTo("data");

    std::vector<int> data = {100, 200, 300};
    sender.SendEvent(TEvent<std::vector<int>>("data", data));

    receiver.PollEvents();
    EXPECT_EQ(receiver.GetLastData(), data);
}

TEST_F(EventComponentTest, ComponentCanAttachBusAfterConstruction) {
    PollingComponent sender;
    PollingComponent receiver;
    std::vector<int> payload = {4, 5, 6};

    sender.SetBus(bus);
    receiver.SetBus(bus);
    receiver.SubscribeTo("late-bind");

    sender.SendEvent(TEvent<std::vector<int>>("late-bind", payload));
    EXPECT_EQ(receiver.PollEvents(), 1u);
    EXPECT_EQ(receiver.GetLastData(), payload);
}

// ---------------------------------------------------------------------------
// A flexible test component that records all received events.
// ---------------------------------------------------------------------------
class RecordingComponent : public ThreadEventComponent {
public:
    explicit RecordingComponent(EventBus& bus) : ThreadEventComponent(bus) {}
    RecordingComponent() = default;

    void Init() override {
        // Subscriptions are set up explicitly per test via SubscribeTo().
    }

    void MainLoop() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void ProcessEvent(Event *incoming) override {
        std::lock_guard<std::mutex> lock(m_recordMutex);
        m_receivedKeys.push_back(incoming->GetKey());

        auto *typed = event_cast<std::vector<int>>(incoming);
        if (typed) {
            m_lastData = typed->GetObject();
        }
    }

    void SubscribeTo(const EventKey& key) {
        Subscribe(key);
    }

    void UnsubscribeFrom(const EventKey& key) {
        Unsubscribe(key);
    }

    std::vector<EventKey> GetReceivedKeys() {
        std::lock_guard<std::mutex> lock(m_recordMutex);
        return m_receivedKeys;
    }

    std::vector<int> GetLastData() {
        std::lock_guard<std::mutex> lock(m_recordMutex);
        return m_lastData;
    }

    size_t GetReceivedCount() {
        std::lock_guard<std::mutex> lock(m_recordMutex);
        return m_receivedKeys.size();
    }

private:
    std::mutex m_recordMutex;
    std::vector<EventKey> m_receivedKeys;
    std::vector<int> m_lastData;
};

// ---------------------------------------------------------------------------
// Fixture — each test gets its own EventBus for isolation.
// ---------------------------------------------------------------------------
class ThreadEventComponentTest : public ::testing::Test {
protected:
    EventBus bus;
};

// ---------------------------------------------------------------------------
// Original tests (kept, with improved assertions)
// ---------------------------------------------------------------------------

TEST_F(ThreadEventComponentTest, ProcessEventDirectCall) {
    RecordingComponent comp(bus);

    std::vector<int> data = {10, 20, 30};
    TEvent<std::vector<int>> ev("some key", data);
    comp.ProcessEvent(&ev);

    EXPECT_EQ(comp.GetReceivedCount(), 1u);
    EXPECT_EQ(comp.GetReceivedKeys()[0], "some key");
    EXPECT_EQ(comp.GetLastData(), data);
}

TEST_F(ThreadEventComponentTest, StartAndStopCleanly) {
    RecordingComponent comp(bus);
    comp.Start();

    // The thread should be running; give it a moment then stop.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    comp.Stop();
    comp.WaitToFinish();

    // After join the thread is no longer running — a second WaitToFinish
    // should be a safe no-op.
    comp.WaitToFinish();
    SUCCEED();
}

TEST_F(ThreadEventComponentTest, ForceStopDrainsQueue) {
    RecordingComponent comp(bus);
    comp.SubscribeTo("flood");
    comp.Start();

    // Flood the queue with events.
    std::vector<int> payload = {1};
    for (int i = 0; i < 50; ++i) {
        comp.SendEvent(TEvent<std::vector<int>>("flood", payload));
    }

    // ForceStop drains the queue before exiting.
    comp.ForceStop();
    comp.WaitToFinish();

    // The component should NOT have processed all 50 events because
    // ForceStop clears the queue.  It may have processed some that were
    // already dequeued before the stop, but not all.
    EXPECT_LT(comp.GetReceivedCount(), 50u);
}

TEST_F(ThreadEventComponentTest, SubscribeAndUnsubscribe) {
    RecordingComponent comp(bus);

    EXPECT_FALSE(comp.IsSubscribed("evt"));
    comp.SubscribeTo("evt");
    EXPECT_TRUE(comp.IsSubscribed("evt"));

    comp.UnsubscribeFrom("evt");
    EXPECT_FALSE(comp.IsSubscribed("evt"));
}

TEST_F(ThreadEventComponentTest, DoubleSubscribeIsIdempotent) {
    RecordingComponent comp(bus);

    comp.SubscribeTo("evt");
    comp.SubscribeTo("evt");  // should not add a second entry
    EXPECT_TRUE(comp.IsSubscribed("evt"));

    // After one unsubscribe it should be fully gone.
    comp.UnsubscribeFrom("evt");
    EXPECT_FALSE(comp.IsSubscribed("evt"));
}

// ---------------------------------------------------------------------------
// Cross-thread event delivery
// ---------------------------------------------------------------------------

TEST_F(ThreadEventComponentTest, SendEventDeliveredAcrossThreads) {
    RecordingComponent sender(bus);
    RecordingComponent receiver(bus);

    receiver.SubscribeTo("ping");
    receiver.Start();

    // Send from the main thread (sender is not started — SendEvent works
    // without a running thread).
    std::vector<int> data = {7, 8, 9};
    sender.SendEvent(TEvent<std::vector<int>>("ping", data));

    // Wait for the receiver thread to pick it up.
    for (int i = 0; i < 50 && receiver.GetReceivedCount() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    receiver.Stop();
    receiver.WaitToFinish();

    ASSERT_EQ(receiver.GetReceivedCount(), 1u);
    EXPECT_EQ(receiver.GetReceivedKeys()[0], "ping");
    EXPECT_EQ(receiver.GetLastData(), data);
}

// ---------------------------------------------------------------------------
// Multi-subscriber fan-out
// ---------------------------------------------------------------------------

TEST_F(ThreadEventComponentTest, MultipleSubscribersReceiveSameEvent) {
    RecordingComponent sender(bus);
    RecordingComponent r1(bus), r2(bus), r3(bus);

    r1.SubscribeTo("broadcast");
    r2.SubscribeTo("broadcast");
    r3.SubscribeTo("broadcast");

    r1.Start();
    r2.Start();
    r3.Start();

    std::vector<int> data = {42};
    sender.SendEvent(TEvent<std::vector<int>>("broadcast", data));

    // Wait for all receivers.
    for (int i = 0; i < 50; ++i) {
        if (r1.GetReceivedCount() >= 1 &&
            r2.GetReceivedCount() >= 1 &&
            r3.GetReceivedCount() >= 1)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    r1.Stop(); r2.Stop(); r3.Stop();
    r1.WaitToFinish(); r2.WaitToFinish(); r3.WaitToFinish();

    EXPECT_EQ(r1.GetReceivedCount(), 1u);
    EXPECT_EQ(r2.GetReceivedCount(), 1u);
    EXPECT_EQ(r3.GetReceivedCount(), 1u);
    EXPECT_EQ(r1.GetLastData(), data);
    EXPECT_EQ(r2.GetLastData(), data);
    EXPECT_EQ(r3.GetLastData(), data);
}

// ---------------------------------------------------------------------------
// Unsubscribed component must NOT receive events
// ---------------------------------------------------------------------------

TEST_F(ThreadEventComponentTest, UnsubscribedComponentDoesNotReceive) {
    RecordingComponent sender(bus);
    RecordingComponent receiver(bus);

    receiver.SubscribeTo("msg");
    receiver.Start();

    // Deliver one event to confirm subscription works.
    std::vector<int> first = {1};
    sender.SendEvent(TEvent<std::vector<int>>("msg", first));

    for (int i = 0; i < 50 && receiver.GetReceivedCount() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    ASSERT_EQ(receiver.GetReceivedCount(), 1u);

    // Unsubscribe, then send another event.
    receiver.UnsubscribeFrom("msg");

    std::vector<int> second = {2};
    sender.SendEvent(TEvent<std::vector<int>>("msg", second));

    // Give time for a possible (incorrect) delivery.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    receiver.Stop();
    receiver.WaitToFinish();

    // Should still have only the first event.
    EXPECT_EQ(receiver.GetReceivedCount(), 1u);
    EXPECT_EQ(receiver.GetLastData(), first);
}

// ---------------------------------------------------------------------------
// Plain (non-template) Event delivery preserves key via clone
// ---------------------------------------------------------------------------

TEST_F(ThreadEventComponentTest, PlainEventDeliveredAcrossThreads) {
    RecordingComponent sender(bus);
    RecordingComponent receiver(bus);

    receiver.SubscribeTo("plain");
    receiver.Start();

    sender.SendEvent(Event("plain"));

    for (int i = 0; i < 50 && receiver.GetReceivedCount() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    receiver.Stop();
    receiver.WaitToFinish();

    ASSERT_EQ(receiver.GetReceivedCount(), 1u);
    EXPECT_EQ(receiver.GetReceivedKeys()[0], "plain");
}

// ---------------------------------------------------------------------------
// event_cast returns nullptr for wrong type
// ---------------------------------------------------------------------------

TEST_F(ThreadEventComponentTest, EventCastReturnsNullptrOnMismatch) {
    std::vector<int> data = {1, 2};
    TEvent<std::vector<int>> ev("key", data);

    // Cast to wrong type should yield nullptr.
    auto *bad = event_cast<std::string>(static_cast<Event*>(&ev));
    EXPECT_EQ(bad, nullptr);

    // Cast to correct type should succeed.
    auto *good = event_cast<std::vector<int>>(static_cast<Event*>(&ev));
    ASSERT_NE(good, nullptr);
    EXPECT_EQ(good->GetObject(), data);
}

// ---------------------------------------------------------------------------
// event_cast reference overload throws on mismatch
// ---------------------------------------------------------------------------

TEST_F(ThreadEventComponentTest, EventCastReferenceThrowsOnMismatch) {
    std::vector<int> data = {1};
    TEvent<std::vector<int>> ev("key", data);
    Event& ref = ev;

    EXPECT_THROW(event_cast<std::string>(ref), std::bad_cast);
    EXPECT_NO_THROW(event_cast<std::vector<int>>(ref));
}

// ---------------------------------------------------------------------------
// clone() preserves TEvent<T> payload through base pointer
// ---------------------------------------------------------------------------

TEST_F(ThreadEventComponentTest, ClonePreservesDerivedPayload) {
    std::vector<int> data = {10, 20, 30};
    TEvent<std::vector<int>> original("key", data);

    // Clone via base class pointer.
    std::unique_ptr<Event> cloned = original.clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->GetKey(), "key");

    auto *typed = event_cast<std::vector<int>>(cloned.get());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->GetObject(), data);
}

TEST_F(ThreadEventComponentTest, CanAttachBusAndStartDetached) {
    RecordingComponent sender(bus);
    RecordingComponent receiver;

    receiver.SetBus(bus);
    receiver.SubscribeTo("detached");
    receiver.Start(true);

    sender.SendEvent(Event("detached"));

    for (int i = 0; i < 50 && receiver.GetReceivedCount() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    receiver.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(receiver.GetReceivedCount(), 1u);
}
