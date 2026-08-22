#pragma once

#include "Logging.hpp"
#include "Utils.hpp"
#include <chrono>

namespace Hermes::core {

class alignas(8) Trade {
public:
  Trade() = default;
  Trade(TradeID tradeID, OrderID makerOrderID, OrderID takerOrderID,
        Price price, Quantity quantity, Side takerSide)
      : m_TradeID(tradeID), m_MakerOrderID(makerOrderID),
        m_TakerOrderID(takerOrderID), m_Price(price), m_Quantity(quantity),
        m_TakerSide(takerSide) {}

  ~Trade() noexcept = default;

  Trade(const Trade &) noexcept = default;
  Trade &operator=(const Trade &) noexcept = default;
  Trade(Trade &&) noexcept = default;
  Trade &operator=(Trade &&) noexcept = default;

  TradeID GetTradeID() const noexcept { return m_TradeID; }
  OrderID GetMakerOrderID() const noexcept { return m_MakerOrderID; }
  OrderID GetTakerOrderID() const noexcept { return m_TakerOrderID; }
  Price GetPrice() const noexcept { return m_Price; };
  Quantity GetQuantity() const noexcept { return m_Quantity; }
  Side GetTakerSide() const noexcept { return m_TakerSide; }
  std::chrono::nanoseconds GetTimestamp() const noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        m_Timestamp.time_since_epoch());
  }

  void log() const {
    LOG_INFO("TradeID: {}, Maker OrderID: {}, Taker OrderID: {}, Price: {}, "
             "Quantity: {}, Taker Side: {}, Timestamp: {}\n",
             m_TradeID, m_MakerOrderID, m_TakerOrderID, m_Price, m_Quantity,
             core::to_string(m_TakerSide), GetTimestamp());
  }

private:
  TradeID m_TradeID;
  OrderID m_MakerOrderID;
  OrderID m_TakerOrderID;
  Price m_Price;
  Quantity m_Quantity;
  Side m_TakerSide;

  Clock::time_point m_Timestamp = Clock::now();
};
} // namespace Hermes::core
