#include "Data/SPSC_Queue.hpp"

#include <atomic>
#include <chrono>
#include <print>
#include <thread>

int main() {
  using Queue = Hermes::data::SPSC_Queue<int, 1024>;
  using clock = std::chrono::steady_clock;

  constexpr int iter = 100'000'000;
  Queue queue;

  std::atomic<bool> start{false};
  std::uint64_t checksum = 0;

  std::jthread prod([&] {
    while (!start.load(std::memory_order_acquire)) {
    }

    for (int i = 0; i < iter; i++)
      while (!queue.try_push(i)) {
      }
  });

  std::jthread consumer([&]() {
    while (!start.load(std::memory_order_acquire)) {
    }

    for (int i = 0; i < iter;) {
      int val{};

      if (!queue.try_pop(val))
        continue;

      checksum += val;
      i++;
    }
  });

  const auto begin = clock::now();

  start.store(true, std::memory_order_release);
  prod.join();
  consumer.join();

  const auto end = clock::now();

  const auto elapsed = std::chrono::duration<double>(end - begin);

  const double seconds = elapsed.count();
  const double elements_per_second = iter / seconds;
  const double ns_per_element = (seconds * 1'000'000'000.0) / iter;

  std::println("SPSC Queue Benchmark");
  std::println("===================");
  std::println("Elements: {:4}", iter);
  std::println("Elapsed: {:4}", seconds);
  std::println("Throughput: {:4} million elements/s",
               elements_per_second / 1'000'000.0);
  std::println("Time per element: {:4} ns", ns_per_element);
  std::println("Checksum: {:4}", checksum);

  return 0;
}
