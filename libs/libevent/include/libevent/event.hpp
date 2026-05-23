#ifndef EVENT_HPP
#define EVENT_HPP

#include <string>
#include <memory>
#include <stdexcept>

namespace grid::libevent {

typedef std::string EventKey;

class Event
{
public:
  Event()
      : m_key()
  {
  }

  Event(EventKey key)
      : m_key(key)
  {
  }

  Event(const Event &e)
      : m_key(e.m_key)
  {
  }

  virtual ~Event()
  {
  }

  EventKey GetKey() const
  {
    return m_key;
  }

  virtual std::unique_ptr<Event> clone() const
  {
    return std::make_unique<Event>(*this);
  }

protected:
  EventKey m_key;
};

template <class T>
class TEvent : public Event
{
public:
  TEvent()
      : Event()
  {
  }

  TEvent(EventKey type, T &object)
      : Event(type), m_object(object)
  {
  }

  TEvent(const TEvent<T> &e)
      : Event(e.m_key), m_object(e.m_object)
  {
  }

  virtual ~TEvent()
  {
  }

  T &GetObject()
  {
    return m_object;
  }

  std::unique_ptr<Event> clone() const override
  {
    return std::make_unique<TEvent<T>>(*this);
  }

private:
  T m_object;
};

/// Type-safe event cast. Pointer overload returns nullptr on type mismatch.
template <class T>
TEvent<T>* event_cast(Event* e)
{
  return dynamic_cast<TEvent<T>*>(e);
}

/// Type-safe event cast. Reference overload throws std::bad_cast on type mismatch.
template <class T>
TEvent<T>& event_cast(Event& e)
{
  auto* typed = dynamic_cast<TEvent<T>*>(&e);
  if (!typed) {
    throw std::bad_cast();
  }
  return *typed;
}

} // namespace grid::libevent

#endif // EVENT_HPP
