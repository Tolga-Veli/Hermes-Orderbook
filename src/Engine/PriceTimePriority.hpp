#pragma once

#include "MatchingPolicy.hpp"

#include <algorithm>

namespace Hermes::engine::matching {
class PriceTimePriority {
public:
  void match(MatchingContext &context) const noexcept {
    ob::OrderNode *node = context.level.Front();

    while (node != nullptr && context.incoming.GetRemainingQuantity() > 0) {
      ob::OrderNode *next = node->next;

      const Quantity quantity =
          std::min(context.incoming.GetRemainingQuantity(),
                   node->order.GetRemainingQuantity());

      context.execute(context.incoming, *node, quantity);

      node = next;
    }
  }
};
} // namespace Hermes::engine::matching
