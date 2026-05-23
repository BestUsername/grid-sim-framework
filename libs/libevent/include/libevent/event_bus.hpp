#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "libevent/event.hpp"

namespace grid::libevent {

class EventComponent;

/// An explicit pub/sub event bus.  Components register with a bus instance
/// rather than a process-global static map, enabling multiple isolated
/// event domains in the same process (e.g. independent engines, tests).
class EventBus
{
public:
    EventBus() = default;
    ~EventBus() = default;

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    void subscribe(EventComponent* comp, EventKey key);
    void unsubscribe(EventComponent* comp, EventKey key);
    bool isSubscribed(EventComponent* comp, EventKey key);

    /// Send an event to all subscribers of the event's key.
    /// Uses clone() to give each subscriber an independent copy.
    void sendEvent(const Event& e);

private:
    using SubscriberList = std::vector<EventComponent*>;
    using EventMap = std::map<EventKey, SubscriberList>;

    EventMap m_subscribers;
    std::mutex m_mutex;
};

} // namespace grid::libevent

#endif // EVENT_BUS_HPP
