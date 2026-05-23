#include "libevent/event_component.hpp"

#include <algorithm>
#include <cassert>

namespace grid::libevent {

EventComponent::EventComponent(EventBus& bus)
    : m_bus(&bus)
{
}

EventComponent::EventComponent()
    : m_bus(nullptr)
{
}

EventComponent::~EventComponent()
{
    if (m_bus) {
        for (const auto& key : m_subscribedKeys) {
            m_bus->unsubscribe(this, key);
        }
    }
}

size_t EventComponent::PollEvents()
{
    size_t count = 0;
    while (true)
    {
        std::unique_ptr<Event> event;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_eventQueue.empty())
                break;
            event = std::move(m_eventQueue.front());
            m_eventQueue.pop();
        }
        ProcessEvent(event.get());
        ++count;
    }
    return count;
}

void EventComponent::SendEvent(const Event &e)
{
    assert(m_bus && "EventComponent has no bus");
    m_bus->sendEvent(e);
}

void EventComponent::Subscribe(EventKey key)
{
    assert(m_bus && "EventComponent has no bus");
    m_bus->subscribe(this, key);
    if (std::find(m_subscribedKeys.begin(), m_subscribedKeys.end(), key) == m_subscribedKeys.end()) {
        m_subscribedKeys.push_back(key);
    }
}

void EventComponent::Unsubscribe(EventKey key)
{
    assert(m_bus && "EventComponent has no bus");
    m_bus->unsubscribe(this, key);
    m_subscribedKeys.erase(
        std::remove(m_subscribedKeys.begin(), m_subscribedKeys.end(), key),
        m_subscribedKeys.end());
}

bool EventComponent::IsSubscribed(EventKey key)
{
    assert(m_bus && "EventComponent has no bus");
    return m_bus->isSubscribed(this, key);
}

void EventComponent::addEvent(const Event &e)
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_eventQueue.push(e.clone());
    }
    m_queueCV.notify_one();
}

} // namespace grid::libevent
