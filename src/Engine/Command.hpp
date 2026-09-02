#pragma once

#include "Core/Utils.hpp"

namespace Hermes::engine {

enum class CommandType : u8 { NewOrder, CancelOrder, ModifyOrder };

struct NewOrder {
  Price price;
  Quantity qty;
  Side side;
  OrderType type;
  TimeInForce tif;
};

struct CancelOrder {
  OrderID orderID;
  Side side;
};

struct ModifyOrder {
  OrderID orderID;
  Price price;
  Quantity quantity;
  Side side;
};

struct Command {
  CommandType type;

  union CommandPayload {
    NewOrder new_order;
    CancelOrder cancel_order;
    ModifyOrder modify_order;
  };

  CommandPayload payload;

  OrderID GetOrderID() const noexcept {
    switch (type) {
    case CommandType::CancelOrder:
      return payload.cancel_order.orderID;
    case CommandType::ModifyOrder:
      return payload.modify_order.orderID;
    default:
      return 0;
    }
  }
};

struct CommandEnvelope {
  ClientID clientID;
  Command cmd;
  SequenceID seq;
  bool rejected{false};
};

} // namespace Hermes::engine
