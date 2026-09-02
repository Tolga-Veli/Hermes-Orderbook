#pragma once

#include "Core/Assert.hpp"
#include "Core/Order.hpp"
#include "Core/Utils.hpp"

#include <cstddef>
#include <utility>

namespace Hermes::engine::ob {
class PriceLevel;

// Intrusive list node wrapping a resting order. Lives inside a NodePool
struct OrderNode {
  core::Order order;
  OrderNode *prev{}, *next{};
  PriceLevel *level{};

  template <class... Args>
  explicit OrderNode(Args &&...args) : order(std::forward<Args>(args)...) {}
};

class PriceLevel {
public:
  explicit PriceLevel(Price price) noexcept : m_Price(price) {}

  Price GetPrice() const noexcept { return m_Price; }
  Quantity GetTotalQuantity() const noexcept { return m_TotalQuantity; }
  std::size_t OrderCount() const noexcept { return m_Count; }
  bool Empty() const noexcept { return m_Head == nullptr; }

  OrderNode *Front() noexcept { return m_Head; }
  const OrderNode *Front() const noexcept { return m_Head; };

  void PushBack(OrderNode *node) noexcept {
    node->prev = m_Tail;
    node->next = nullptr;

    if (m_Tail)
      m_Tail->next = node;
    else
      m_Head = node;

    m_Tail = node;
    node->level = this;
    m_TotalQuantity += node->order.GetRemainingQuantity();
    ++m_Count;
  }

  // Unlinks node from this level.
  bool Erase(OrderNode *node) noexcept {
    HERMES_ASSERT(node->level == this, "Erasing node from wrong level");

    // Fail to erase, doesn't corrupt OrderBook state
    if (node->level != this)
      return false;

    if (node->prev)
      node->prev->next = node->next;
    else
      m_Head = node->next;

    if (node->next)
      node->next->prev = node->prev;
    else
      m_Tail = node->prev;

    ReduceQuantity(node->order.GetRemainingQuantity());
    --m_Count;

    node->prev = node->next = nullptr;
    node->level = nullptr;
    return true;
  }

  void ReduceQuantity(Quantity delta) noexcept {
    HERMES_VERIFY(delta <= m_TotalQuantity, "Quantity reduction exceeds total");
    m_TotalQuantity -= delta;
  }

  void IncreaseQuantity(Quantity delta) noexcept { m_TotalQuantity += delta; }

  struct iterator {
    OrderNode *node;

    OrderNode &operator*() const noexcept { return *node; }
    OrderNode *operator->() const noexcept { return node; }
    iterator &operator++() noexcept {
      HERMES_VERIFY(node, "Node was a nullptr");
      node = node->next;
      return *this;
    }

    bool operator==(const iterator &) const noexcept = default;
  };

  iterator begin() noexcept { return {m_Head}; }
  iterator end() noexcept { return {nullptr}; }

  void VerifyIntegrity() const noexcept {
    Quantity total{};
    std::size_t cnt{};
    for (const OrderNode *node = m_Head; node != nullptr; node = node->next)
      total += node->order.GetRemainingQuantity(), ++cnt;

    HERMES_VERIFY(total == m_TotalQuantity, "Price level quantity is invalid");
    HERMES_VERIFY(cnt == m_Count, "Price level order count is invalid");
  }

private:
  Price m_Price{};
  OrderNode *m_Head{}, *m_Tail{};
  Quantity m_TotalQuantity{};
  std::size_t m_Count{};
};
} // namespace Hermes::engine::ob
