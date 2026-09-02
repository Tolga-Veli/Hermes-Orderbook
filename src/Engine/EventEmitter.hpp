#pragma once

#include "Core/Utils.hpp"
#include "Engine/Event.hpp"
#include <thread>

namespace Hermes::engine::events {
template <class EventQueue> class EventEmitter {

public:
  EventEmitter(EventQueue &queue) noexcept : m_Queue(queue) {}

  template <class EventPayload>
  void Emit(EventType type, EventPayload payload) noexcept {
    Event event(type, m_Sequence++, payload);

    while (!m_Queue.try_push(event))
      std::this_thread::yield();
  }

  void BookAdd(Side side, Price price, Quantity quantity) noexcept {
    Emit(EventType::BookAdd,
         Event::EventPayload{
             .book_add = events::BookAdd{
                 .side = side, .price = price, .quantity = quantity}});
  }

  void BookModify(Side side, Price price, Quantity quantity) noexcept {
    Emit(EventType::BookModify,
         Event::EventPayload{.book_mod = events::BookModify{
                                 .side = side,
                                 .price = price,
                                 .quantity = quantity,
                             }});
  }

  void BookDelete(Side side, Price price) noexcept {
    Emit(EventType::BookDelete,
         Event::EventPayload{.book_del = events::BookDelete{
                                 .side = side,
                                 .price = price,
                             }});
  }

  void Trade(const core::Trade &trade) noexcept {
    Emit(EventType::Trade, Event::EventPayload{.trade = trade});
  }

  void OrderAccepted(ClientID client_id, OrderID order_id) noexcept {
    Emit(EventType::OrderAccepted,
         Event::EventPayload{.order_accepted = events::OrderAccepted{
                                 .client_id = client_id,
                                 .order_id = order_id,
                             }});
  }

  void OrderRejected(ClientID client_id, OrderID order_id) noexcept {
    Emit(EventType::OrderRejected,
         Event::EventPayload{.order_rejected = events::OrderRejected{
                                 .client_id = client_id,
                                 .order_id = order_id,
                             }});
  }

  void OrderCancelled(ClientID client_id, OrderID order_id) noexcept {
    Emit(EventType::OrderCancelled,
         Event::EventPayload{.order_cancelled = events::OrderCancelled{
                                 .client_id = client_id,
                                 .order_id = order_id,
                             }});
  }

  void OrderExecuted(ClientID client_id, OrderID order_id, Price price,
                     Quantity quantity) noexcept {
    Emit(EventType::OrderExecuted,
         Event::EventPayload{.order_executed = events::OrderExecuted{
                                 .client_id = client_id,
                                 .order_id = order_id,
                                 .price = price,
                                 .quantity = quantity,
                             }});
  }

  void OrderPartiallyFilled(ClientID client_id, OrderID order_id, Price price,
                            Quantity quantity) noexcept {
    Emit(EventType::OrderPartiallyFilled,
         Event::EventPayload{.order_partially_filled =
                                 events::OrderPartiallyFilled{
                                     .client_id = client_id,
                                     .order_id = order_id,
                                     .price = price,
                                     .quantity = quantity,
                                 }});
  }

  void OrderModified(ClientID client_id, OrderID order_id) noexcept {
    Emit(EventType::OrderModified,
         Event::EventPayload{.order_modified = events::OrderModified{
                                 .client_id = client_id,
                                 .order_id = order_id,
                             }});
  }

private:
  EventQueue &m_Queue;
  SequenceID m_Sequence{1};
};
} // namespace Hermes::engine::events
