#pragma once

#include "Assert.hpp"
#include "Utils.hpp"
#include <chrono>

namespace Hermes::core {
class Order final {
public:
  Order() = delete;
  Order(OrderID orderID, ClientID clientID, Price price, Quantity quantity,
        Side side, OrderType order_type, TimeInForce tif) noexcept
      : m_OrderID(orderID), m_ClientID(clientID), m_Price(price),
        m_IntialQuantity(quantity), m_RemainingQuantity(quantity), m_Side(side),
        m_OrderType(order_type), m_TimeInForce(tif) {}

  ~Order() noexcept = default;

  Order(const Order &) noexcept = default;
  Order(Order &&) noexcept = default;
  Order &operator=(const Order &) noexcept = default;
  Order &operator=(Order &&) noexcept = default;

  OrderID GetOrderID() const noexcept { return m_OrderID; }
  ClientID GetClientID() const noexcept { return m_ClientID; }
  Price GetPrice() const noexcept { return m_Price; }
  Quantity GetInitialQuantity() const noexcept { return m_IntialQuantity; }
  Quantity GetRemainingQuantity() const noexcept { return m_RemainingQuantity; }
  Quantity GetFilledQuantity() const noexcept {
    return m_IntialQuantity - m_RemainingQuantity;
  }
  std::chrono::nanoseconds GetTimestamp() const noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        m_Timestamp.time_since_epoch());
  }
  Side GetSide() const noexcept { return m_Side; }
  OrderType GetOrderType() const noexcept { return m_OrderType; }
  TimeInForce GetTimeInForce() const noexcept { return m_TimeInForce; }
  bool isFilled() const noexcept { return m_RemainingQuantity == 0; }

  void ModifyOrder(Price new_price, Quantity new_quantity) noexcept {
    const Quantity filled = GetFilledQuantity();

    // NOTE: crash program instead of allowing corrupted state
    HERMES_VERIFY(
        new_quantity >= filled,
        "Modified quantity is less than the already filled quantity.");

    if (new_price != m_Price || new_quantity > m_IntialQuantity)
      m_Timestamp = Clock::now();

    m_Price = new_price;
    m_RemainingQuantity = new_quantity - filled;
    m_IntialQuantity = new_quantity;
  }

  void Fill(Quantity quantity) noexcept {
    // NOTE: crash program instead of allowing corrupted state
    HERMES_VERIFY(quantity <= m_RemainingQuantity,
                  "Order cannot be filled for more than its capacity");

    m_RemainingQuantity -= quantity;
  }

  void log() const {
    LOG_INFO("Order ID: {}, ClientID: {}, Price: {}, InitialQuantity: {}, "
             "RemainingQuantity: {}, Time: {}, Side: {}, Order Type: {}, "
             "Time in Force: {} \n",
             m_OrderID, m_ClientID, m_Price, m_IntialQuantity,
             m_RemainingQuantity, GetTimestamp(), core::to_string(m_Side),
             core::to_string(m_OrderType), core::to_string(m_TimeInForce));
  }

private:
  OrderID m_OrderID;
  ClientID m_ClientID;
  Price m_Price;
  Quantity m_IntialQuantity, m_RemainingQuantity;
  Side m_Side;
  OrderType m_OrderType;
  TimeInForce m_TimeInForce;

  Clock::time_point m_Timestamp = Clock::now();
};
} // namespace Hermes::core
