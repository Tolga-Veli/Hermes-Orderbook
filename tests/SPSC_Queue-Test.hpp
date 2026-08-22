#include "Data/SPSC_Queue.hpp"

#include <cstdint>
#include <gtest/gtest.h>

using Hermes::data::SPSC_Queue;

TEST(SPSC_Queue, StartsEmpty) {
  SPSC_Queue<int, 8> queue;

  EXPECT_TRUE(queue.empty());
  EXPECT_FALSE(queue.full());
}

TEST(SPSC_Queue, PushAndPop) {
  SPSC_Queue<int, 8> queue;

  ASSERT_TRUE(queue.try_push(42));

  int val{};
  ASSERT_TRUE(queue.try_pop(val));
  EXPECT_EQ(val, 42);

  EXPECT_TRUE(queue.empty());
  EXPECT_FALSE(queue.full());
}

TEST(SPSC_Queue, PopEmptyFails) {
  SPSC_Queue<int, 8> queue;

  int val{123};
  EXPECT_FALSE(queue.try_pop(val));

  EXPECT_EQ(val, 123);
  EXPECT_TRUE(queue.empty());
}

TEST(SPSC_Queue, FIFO) {
  SPSC_Queue<int, 8> queue;

  for (int i = 0; i < 7; i++)
    ASSERT_TRUE(queue.try_push(i));

  EXPECT_TRUE(queue.full());
  EXPECT_FALSE(queue.try_push(7));

  for (int i = 0; i < 7; i++) {
    int val{};

    ASSERT_TRUE(queue.try_pop(val));
    EXPECT_EQ(val, i);
  }

  EXPECT_TRUE(queue.empty());
}

TEST(SPSC_Queue, ReusesSlots) {
  SPSC_Queue<int, 8> queue;

  for (int i = 0; i < 7; i++)
    ASSERT_TRUE(queue.try_push(i));

  for (int i = 0; i < 7; i++) {
    int val{};
    ASSERT_TRUE(queue.try_pop(val));
    EXPECT_EQ(val, i);
  }

  EXPECT_TRUE(queue.empty());

  for (int i = 7; i < 14; i++)
    ASSERT_TRUE(queue.try_push(i));

  for (int exp = 7; exp < 14; exp++) {
    int val{};
    ASSERT_TRUE(queue.try_pop(val));
    EXPECT_EQ(val, exp);
  }

  EXPECT_TRUE(queue.empty());
}

TEST(SPSC_Queue, MultipleWrapArounds) {
  SPSC_Queue<int, 8> queue;

  constexpr int iter = 10000;
  for (int i = 0; i < iter; i++) {
    int base = iter * 7;

    for (int j = 0; j < 7; j++)
      ASSERT_TRUE(queue.try_push(base + i));

    EXPECT_TRUE(queue.full());

    for (int j = 0; j < 7; j++) {
      int val{};

      ASSERT_TRUE(queue.try_pop(val));
      EXPECT_EQ(val, base + i);
    }
    EXPECT_TRUE(queue.empty());
  }
}

TEST(SPSC_Queue, FailedPush) {
  SPSC_Queue<int, 8> queue;

  for (int i = 0; i < 7; i++)
    ASSERT_TRUE(queue.try_push(i));

  EXPECT_TRUE(queue.full());
  EXPECT_FALSE(queue.try_push(999));

  for (int exp = 0; exp < 7; exp++) {
    int val{};

    ASSERT_TRUE(queue.try_pop(val));
    EXPECT_EQ(val, exp);
  }

  EXPECT_TRUE(queue.empty());
}

TEST(SPSC_Queue, Objects) {
  struct TestObj {
    int id, price, quantity;

    bool operator==(const TestObj &) const = default;
  };

  SPSC_Queue<TestObj, 8> queue;

  const TestObj order{.id = 123, .price = 105000, .quantity = 10};

  ASSERT_TRUE(queue.try_push(order));

  TestObj res{};

  ASSERT_TRUE(queue.try_pop(res));
  EXPECT_EQ(res, order);
}

TEST(SPSC_Queue, ProperForwarding) {
  SPSC_Queue<std::string, 8> queue;

  ASSERT_TRUE(queue.try_push(std::string{"hello"}));

  std::string val;

  ASSERT_TRUE(queue.try_pop(val));
  EXPECT_EQ(val, "hello");
}

#include <atomic>
#include <thread>

TEST(SPSC_Queue, Concurrency) {
  using u64 = std::uint64_t;
  SPSC_Queue<u64, 1024> queue;
  constexpr u64 count = 1e6;

  std::atomic<bool> prod_done = false, failed = false;

  std::jthread prod([&] {
    for (u64 i = 0; i < count; i++)
      while (!queue.try_push(i))
        std::this_thread::yield();

    prod_done.store(true, std::memory_order_release);
  });

  std::jthread consumer([&] {
    u64 exp = 0;

    while (exp < count) {
      u64 val{};

      if (!queue.try_pop(val)) {
        std::this_thread::yield();
        continue;
      }

      if (val != exp) {
        failed.store(true, std::memory_order_release);
        return;
      }

      exp++;
    }
  });

  prod.join();
  consumer.join();

  EXPECT_FALSE(failed.load(std::memory_order_acquire));
  EXPECT_TRUE(queue.empty());
}
