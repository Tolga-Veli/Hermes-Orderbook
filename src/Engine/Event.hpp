#pragma once

#include "Core/Trade.hpp"
#include "Core/Utils.hpp"

namespace Hermes::engine {
enum class EventType : u8 {
  // Public:
  None = 0,
  BookAdd,
  BookModify,
  BookDelete,
  Trade,

  PUBLIC_EVENTS_COUNT,

  OrderAccepted,
  OrderRejected,
  OrderCancelled,
  OrderExecuted,
  OrderPartiallyFilled,
  OrderModified,

  ALL_EVENTS_COUNT
};

namespace events {
struct BookAdd {
  Side side;
  Price price;
  Quantity quantity;
};

struct BookModify {
  Side side;
  Price price;
  Quantity quantity;
};

struct BookDelete {
  Side side;
  Price price;
};

struct OrderAccepted {
  ClientID client_id;
  OrderID order_id;
};

struct OrderRejected {
  ClientID client_id;
  OrderID order_id;
};

struct OrderCancelled {
  ClientID client_id;
  OrderID order_id;
};

struct OrderExecuted {
  ClientID client_id;
  OrderID order_id;
  Price price;
  Quantity quantity;
};

struct OrderPartiallyFilled {
  ClientID client_id;
  OrderID order_id;
  Price price;
  Quantity quantity;
};

struct OrderModified {
  ClientID client_id;
  OrderID order_id;
};
} // namespace events

struct Event {
  Event() = default;

  EventType type = EventType::None;
  SequenceID sequence{};
  // used to validate given event, each event sequence must differ
  // by 1, i.e. 1,2,3,... and not 1,3 (means 2 is lost and local
  // book state is corrupted request a snapshot + replay)

  union EventPayload {
    events::BookAdd book_add;
    events::BookModify book_mod;
    events::BookDelete book_del;
    core::Trade trade;

    events::OrderAccepted order_accepted;
    events::OrderRejected order_rejected;
    events::OrderCancelled order_cancelled;
    events::OrderExecuted order_executed;
    events::OrderPartiallyFilled order_partially_filled;
    events::OrderModified order_modified;
  };

  Event(EventType eventType, SequenceID eventSequence,
        const EventPayload &eventPayload) noexcept
      : type(eventType), sequence(eventSequence), payload(eventPayload) {}

  EventPayload payload{};

  bool IsPublic() const noexcept {
    return type >= EventType::BookAdd &&
           type < EventType::PUBLIC_EVENTS_COUNT;
  }
};
} // namespace Hermes::engine
