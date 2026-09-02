#pragma once

#include <concepts>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace Hermes::data {
template <class T> class MutexQueue {
public:
  MutexQueue() = default;
  ~MutexQueue() noexcept = default;

  template <class U>
    requires(std::constructible_from<T, U>)
  bool push(U &&val) {
    {
      std::scoped_lock lock(m_Mutex);
      m_Queue.push(std::forward<U>(val));
    }
    m_NotEmpty.notify_one();
    return true;
  }

  // blocking until an item is available
  T pop() {
    std::unique_lock lock(m_Mutex);
    m_NotEmpty.wait(lock, [this] { return !m_Queue.empty(); });
    T val = std::move(m_Queue.front());
    m_Queue.pop();
    return val;
  }

  // non-blocking
  std::optional<T> try_pop() {
    std::scoped_lock lock(m_Mutex);
    if (m_Queue.empty())
      return std::nullopt;

    T val = std::move(m_Queue.front());
    m_Queue.pop();
    return val;
  }

private:
  std::queue<T> m_Queue;
  std::mutex m_Mutex;
  std::condition_variable m_NotEmpty;
};
} // namespace Hermes::data
