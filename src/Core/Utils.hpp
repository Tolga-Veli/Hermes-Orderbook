#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace Hermes {
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using ClientID = u64;
using ClientOrderID = u64;
using TradeID = u64;
using OrderID = u64;
using Price = i64; // in 1/10th of a cent therefore  1000 = 1$
using Quantity = u64;
using SequenceID = u64;

using Clock = std::chrono::steady_clock;

enum class Side : u8 { Buy = 0, Sell = 1 };

enum class OrderType : u8 {
  Limit = 0,
  Market,
  Stop,
};

enum class TimeInForce : u8 {
  GTC = 0, // Good 'Till Cancelled
  Day, // Day order - expires automatically at the end of the trading session
  IOC, // Immediate-Or-Cancel (fills what it can immediately, cancels rest) -
};

namespace core {
inline constexpr std::string_view to_string(Side side) {
  if (side == Side::Buy)
    return "Buy";
  else
    return "Sell";
}

inline constexpr std::string_view to_string(OrderType order_type) {
  using enum OrderType;
  switch (order_type) {
  case Limit:
    return "Limit";
  case Market:
    return "Market";
  case Stop:
    return "Stop";
  default:
    return "";
  }
}

inline constexpr std::string_view to_string(TimeInForce tif) {
  using enum TimeInForce;
  switch (tif) {
  case Day:
    return "DayOrder";
  case GTC:
    return "GoodTillCancelled";
  case IOC:
    return "ImmediateOrCancel";
  default:
    return "";
  }
}

constexpr bool IsAPowOfTwo(std::size_t val) {
  return val != 0 && (val & (val - 1)) == 0;
}
} // namespace core
} // namespace Hermes
