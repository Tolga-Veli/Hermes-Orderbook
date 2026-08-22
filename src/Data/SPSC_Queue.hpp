#pragma once

#include "Core/Utils.hpp"

#include <atomic>
#include <concepts>
#include <cstddef>

namespace Hermes::data {
template <class T, std::size_t N>
  requires(N > 1 && core::IsAPowOfTwo(N))
class SPSC_Queue {
public:
  SPSC_Queue() noexcept = default;
  ~SPSC_Queue() noexcept = default;

  SPSC_Queue(const SPSC_Queue &) = delete;
  SPSC_Queue &operator=(const SPSC_Queue &) = delete;
  SPSC_Queue(SPSC_Queue &&) = delete;
  SPSC_Queue &operator=(SPSC_Queue &&) = delete;

  template <class U>
    requires(std::constructible_from<T, U>)
  bool try_push(U &&val) noexcept {
    const std::size_t head = m_Head.load(std::memory_order_relaxed),
                      next = (head + 1) & mask;

    if (next == m_TailCache) {
      m_TailCache = m_Tail.load(std::memory_order_acquire);
      if (next == m_TailCache)
        return false;
    }

    m_Buffer[head] = std::forward<U>(val); // maybe head & mask
    m_Head.store(next, std::memory_order_release);
    return true;
  }

  bool try_pop(T &val) noexcept {
    const std::size_t tail = m_Tail.load(std::memory_order_relaxed);

    if (tail == m_HeadCache) {
      m_HeadCache = m_Head.load(std::memory_order_acquire);
      if (m_HeadCache == tail)
        return false;
    }

    val = m_Buffer[tail]; // maybe tail & mask
    m_Tail.store((tail + 1) & mask, std::memory_order_release);
    return true;
  }

  [[nodiscard]] bool empty() const noexcept {
    return m_Head.load(std::memory_order_acquire) ==
           m_Tail.load(std::memory_order_acquire);
  }

  [[nodiscard]]
  bool full() const noexcept {
    const std::size_t head = m_Head.load(std::memory_order_acquire),
                      tail = m_Tail.load(std::memory_order_acquire);

    return ((head + 1) & mask) == tail;
  }

private:
  static constexpr std::size_t mask = N - 1;

  std::array<T, N> m_Buffer{};

  // Producer state
  alignas(64) std::atomic<std::size_t> m_Head{};
  alignas(64) std::size_t m_TailCache{};

  // Consumer state
  alignas(64) std::atomic<std::size_t> m_Tail{};
  alignas(64) std::size_t m_HeadCache{};
};
} // namespace Hermes::data
