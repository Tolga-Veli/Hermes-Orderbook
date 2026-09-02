#pragma once

#include "Core/Assert.hpp"
#include "Core/Order.hpp"
#include "Engine/PriceLevel.hpp"

#include <map>
#include <unordered_map>

namespace Hermes::engine::ob {

// Price levels are kept in an std::map ordered so best price according to
// Compare is always begin()
// O(log levels) per new price level.
//
// TODO: If your instruments has a bounded price range
// flat vector indexed by tick gets this to O(1)
template <Side side> class OrderBookSide {
  using Compare = std::conditional_t<side == Side::Buy, std::greater<Price>,
                                     std::less<Price>>;

public:
  // Inserts an already-constructed node (owned by the caller's pool) into its
  // price level
  void Insert(OrderNode *node) noexcept {
    const Price price = node->order.GetPrice();
    auto [it, _] = m_Levels.try_emplace(price, price);
    it->second.PushBack(node);
    m_OrderIndex.emplace(node->order.GetOrderID(), node);
  }

  // Unlinks the node from its level (erasing the level if its empty)
  // NOTE: Does not release the node back to a pool
  void Remove(OrderNode *node) noexcept {
    PriceLevel *level = node->level;
    HERMES_ASSERT(level, "Node is not on this side");

    const Price price = level->GetPrice();
    level->Erase(node);
    m_OrderIndex.erase(node->order.GetOrderID());

    if (level->Empty())
      m_Levels.erase(price);
  }

  OrderNode *Find(OrderID id) const noexcept {
    auto it = m_OrderIndex.find(id);
    return (it == m_OrderIndex.end() ? nullptr : it->second);
  }

  bool Empty() const noexcept { return m_Levels.empty(); }

  PriceLevel *BestLevel() noexcept {
    return (m_Levels.empty() ? nullptr : &m_Levels.begin()->second);
  }

  std::size_t OrderCount() const noexcept { return m_OrderIndex.size(); }
  std::size_t LevelCount() const noexcept { return m_Levels.size(); }

  void VerifyIntegrity() const noexcept {
    std::size_t cnt{};
    for (const auto &[price, level] : m_Levels) {
      HERMES_VERIFY(price == level.GetPrice(),
                    "Level stored under wrong price key");
      level.VerifyIntegrity();
      cnt += level.OrderCount();
    }

    HERMES_VERIFY(cnt == m_OrderIndex.size(),
                  "Order index size disagrees with sum of level order counts");
  }

private:
  std::map<Price, PriceLevel, Compare> m_Levels;
  std::unordered_map<OrderID, OrderNode *> m_OrderIndex;
};
} // namespace Hermes::engine::ob
