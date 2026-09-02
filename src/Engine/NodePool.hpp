#pragma once

#include "Core/Assert.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace Hermes::engine {
// Fixed capacity free-list pool. Acquire/Release are O(1), branch-light, 1 new
// and 1 delete calls. Not thread-safe by design - each pool is meant to owned
// exclusively by the single thread running th orderbook

template <class T> class NodePool {
  union Slot {
    Slot *next{};
    alignas(T) std::byte storage[sizeof(T)];

    Slot() noexcept = default;
  };

public:
  explicit NodePool(std::size_t cap) : m_Storage(cap) {
    HERMES_VERIFY(cap > 0, "Capacity of node pool must be larger than zero");
    for (std::size_t i = 0; i + 1 < cap; i++)
      m_Storage[i].next = &m_Storage[i + 1];

    m_FreeList = cap > 0 ? &m_Storage[0] : nullptr;
  }

  NodePool(const NodePool &) = delete;
  NodePool &operator=(const NodePool &) = delete;
  NodePool(NodePool &&) = delete;
  NodePool &operator=(NodePool &&) = delete;

  template <class... Args> [[nodiscard]] T *Acquire(Args &&...args) {
    if (m_FreeList == nullptr)
      return nullptr;

    Slot *slot = m_FreeList;
    m_FreeList = slot->next;
    return std::construct_at(reinterpret_cast<T *>(slot->storage),
                             std::forward<Args>(args)...);
  }

  void Release(T *ptr) noexcept {
    std::destroy_at(ptr);
    Slot *slot = reinterpret_cast<Slot *>(reinterpret_cast<std::byte *>(ptr));
    slot->next = m_FreeList;
    m_FreeList = slot;
  }

  std::size_t Capacity() const noexcept { return m_Storage.size(); }

private:
  std::vector<Slot> m_Storage;
  Slot *m_FreeList{};
};
} // namespace Hermes::engine
