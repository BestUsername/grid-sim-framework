#include "libevent/event_component.hpp"

namespace grid::libevent {

void EventBus::subscribe(EventComponent* comp, EventKey key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& subs = m_subscribers[key];
    if (std::find(subs.begin(), subs.end(), comp) == subs.end()) {
        subs.push_back(comp);
    }
}

void EventBus::unsubscribe(EventComponent* comp, EventKey key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto mapIt = m_subscribers.find(key);
    if (mapIt == m_subscribers.end()) return;
    auto elIt = std::find(mapIt->second.begin(), mapIt->second.end(), comp);
    if (elIt == mapIt->second.end()) return;
    mapIt->second.erase(elIt);
}

bool EventBus::isSubscribed(EventComponent* comp, EventKey key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto mapIt = m_subscribers.find(key);
    if (mapIt == m_subscribers.end()) return false;
    return std::find(mapIt->second.begin(), mapIt->second.end(), comp) != mapIt->second.end();
}

void EventBus::sendEvent(const Event& e)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    EventKey key = e.GetKey();
    auto mapIt = m_subscribers.find(key);
    if (mapIt == m_subscribers.end()) return;
    for (auto* comp : mapIt->second) {
        assert(comp != nullptr);
        comp->addEvent(e);
    }
}

} // namespace grid::libevent
