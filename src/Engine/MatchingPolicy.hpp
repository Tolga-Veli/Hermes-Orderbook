#pragma once

#include "Core/FunctionRef.hpp"
#include "Core/Order.hpp"
#include "Core/Utils.hpp"

#include "Engine/PriceLevel.hpp"

#include <concepts>

namespace Hermes::engine::matching {
using ExecuteTradeCallback =
    core::FunctionRef<void(core::Order &, ob::OrderNode &, Quantity)>;

struct MatchingContext {
  core::Order &incoming;
  ob::PriceLevel &level;
  ExecuteTradeCallback execute;
};

template <class P>
concept MatchingPolicy = requires(P policy, MatchingContext &context) {
  { policy.match(context) } noexcept -> std::same_as<void>;
};
} // namespace Hermes::engine::matching
