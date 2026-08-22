#pragma once

#include <gtest/gtest.h>
#include "Orderbook.hpp"

namespace ob {
namespace test {
class OrderbookTest : public testing::Test {
protected:
  engine::Orderbook book;
  void SetUp() override;
  void TearDown() override;

public:
private:
  bool TryAddingOrder();
  bool TryModifyingOrder();
  bool TryCancellingOrder();
  bool TryCancellingAllOrders();
  bool TryChaningMatchingStrategy();
};
}; // namespace test
} // namespace ob