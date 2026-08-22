#include "MatchingPolicy.hpp"

namespace Hermes::engine::matching {
class PriceTimePriority {
public:
  void match(MatchingContext &context) const noexcept {
    for (auto &resting : context.level) {
      if (context.incoming.GetRemainingQuantity() == 0)
        break;

      Quantity quantity = std::min(context.incoming.GetRemainingQuantity(),
                                   resting.GetRemainingQuantity());

      context.execute(context.incoming, resting, quantity);
    }
  }
};
} // namespace Hermes::engine::matching
