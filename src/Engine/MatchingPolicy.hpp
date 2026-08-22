#pragma once

#include "Core/Order.hpp"
#include "Core/Utils.hpp"

#include <span>

namespace Hermes::engine::matching {
struct PriceLevel;

using ExecuteTradeCallback = void (*)(core::Order &, core::Order &,
                                      Quantity) noexcept;

struct MatchingContext {
  core::Order &incoming;
  std::span<core::Order> level;
  ExecuteTradeCallback execute;
};

template <class P>
concept MatchingPolicy = requires(P policy, MatchingContext &context) {
  { policy.match(context) } noexcept -> std::same_as<void>;
};
} // namespace Hermes::engine::matching
