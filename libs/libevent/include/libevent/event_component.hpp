#ifndef EVENT_COMPONENT_HPP
#define EVENT_COMPONENT_HPP

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "libevent/event_bus.hpp"

namespace grid::libevent {

/// Non-threaded base class providing an event queue, pub/sub subscriptions,
/// and synchronous poll-based event processing.  Derive from this when you
/// want to participate in the event system without owning a dedicated thread
/// (e.g. from within an existing game/simulation loop).
///
/// Each EventComponent belongs to an EventBus instance. Components on
/// different buses are completely isolated from one another.
///
/// For a thread-owning variant, see ThreadEventComponent.
class EventComponent
{
public:
    /// Construct with an explicit bus (preferred — no shared global state).
    explicit EventComponent(EventBus& bus);

    /// Construct without a bus.  Subscribe/Unsubscribe/SendEvent/IsSubscribed
    /// will assert-fail unless a bus is later attached via SetBus().
    EventComponent();

    virtual ~EventComponent();

    /// Attach to a bus after construction.
    void SetBus(EventBus& bus) { m_bus = &bus; }

    /// Override to handle incoming events.
    virtual void ProcessEvent(Event *incoming) = 0;

    /// Synchronously drain all pending events, calling ProcessEvent for each.
    /// Returns the number of events processed.
    size_t PollEvents();

    /// Send an event to all subscribers of the event's key.
    /// Uses clone() to give each subscriber an independent copy.
    void SendEvent(const Event &e);

    void Subscribe(EventKey key);
    void Unsubscribe(EventKey key);
    bool IsSubscribed(EventKey key);

protected:
    friend class EventBus;

    /// Enqueue an event (uses clone() to preserve derived type).
    void addEvent(const Event &e);

    std::queue<std::unique_ptr<Event>> m_eventQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV;

    EventBus* m_bus = nullptr;
    std::vector<EventKey> m_subscribedKeys;
};

} // namespace grid::libevent

#endif // EVENT_COMPONENT_HPP
