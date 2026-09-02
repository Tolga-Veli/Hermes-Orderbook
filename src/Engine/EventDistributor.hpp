#pragma once

#include "Core/Utils.hpp"
#include "Event.hpp"

#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Hermes::engine {
class EventDistributor {
public:
  using Sink = std::function<void(const Event &)>;

  void Register(ClientID id, Sink sink) {
    std::scoped_lock lock(m_Mutex);
    m_Sinks[id] = std::move(sink);
  }

  void Unregister(ClientID id) {
    std::scoped_lock lock(m_Mutex);
    m_Sinks.erase(id);
  }

  void Publish(const Event &event) noexcept {
    if (event.IsPublic())
      PublishPublic(event);
    else
      PublishPrivate(event);
  }

private:
  std::mutex m_Mutex;
  std::unordered_map<ClientID, Sink> m_Sinks;

  void PublishPublic(const Event &event) noexcept {
    std::vector<Sink> sinks;
    {
      std::scoped_lock lock(m_Mutex);
      sinks.reserve(m_Sinks.size());
      for (const auto &[_, sink] : m_Sinks)
        sinks.push_back(sink);
    }
    for (const auto &sink : sinks)
      sink(event);
  }

  void PublishPrivate(const Event &event) noexcept {
    ClientID id{};

    using enum EventType;
    switch (event.type) {
    case OrderAccepted:
      id = event.payload.order_accepted.client_id;
      break;

    case OrderRejected:
      id = event.payload.order_rejected.client_id;
      break;

    case OrderCancelled:
      id = event.payload.order_cancelled.client_id;
      break;

    case OrderExecuted:
      id = event.payload.order_executed.client_id;
      break;

    case OrderPartiallyFilled:
      id = event.payload.order_partially_filled.client_id;
      break;

    case OrderModified:
      id = event.payload.order_modified.client_id;
      break;

    default:
      return;
    }

    Sink sink;
    {
      std::scoped_lock lock(m_Mutex);
      auto it = m_Sinks.find(id);
      if (it == m_Sinks.end())
        return;
      sink = it->second;
    }
    sink(event);
  }
};
} // namespace Hermes::engine
