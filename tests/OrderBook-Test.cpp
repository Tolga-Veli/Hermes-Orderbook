#include "Engine/OrderBook.hpp"
#include "Engine/PriceTimePriority.hpp"
#include "Engine/TradingEngine.hpp"

#include <gtest/gtest.h>
#include <vector>

namespace {
using namespace Hermes;
using Book = engine::ob::OrderBook<engine::matching::PriceTimePriority>;

struct Events {
  std::vector<core::Trade> trades;
  std::vector<OrderID> accepted;
  std::vector<OrderID> cancelled;
  std::vector<OrderID> modified;

  void Trade(const core::Trade &trade) { trades.push_back(trade); }
  void OrderAccepted(ClientID, OrderID id) { accepted.push_back(id); }
  void OrderCancelled(ClientID, OrderID id) { cancelled.push_back(id); }
  void OrderModified(ClientID, OrderID id) { modified.push_back(id); }
};

constexpr auto limit = OrderType::Limit;
constexpr auto gtc = TimeInForce::GTC;

TEST(OrderBook, MatchesAtMakerPriceAndRemovesFilledOrders) {
  Book book({}, 4);
  Events events;

  EXPECT_EQ(book.AddOrder(1, 10, 100, 5, Side::Sell, limit, gtc, events),
            engine::ob::ErrorCode::None);
  EXPECT_EQ(book.AddOrder(2, 20, 110, 3, Side::Buy, limit, gtc, events),
            engine::ob::ErrorCode::None);

  ASSERT_EQ(events.trades.size(), 1);
  EXPECT_EQ(events.trades[0].GetMakerOrderID(), 1);
  EXPECT_EQ(events.trades[0].GetTakerOrderID(), 2);
  EXPECT_EQ(events.trades[0].GetPrice(), 100);
  EXPECT_EQ(events.trades[0].GetQuantity(), 3);
  EXPECT_EQ(book.AskOrderCount(), 1);
  EXPECT_EQ(book.BidOrderCount(), 0);
  book.VerifyIntegrity();
}

TEST(OrderBook, UsesPriceThenTimePriority) {
  Book book({}, 8);
  Events events;
  ASSERT_EQ(book.AddOrder(1, 1, 101, 2, Side::Sell, limit, gtc, events),
            engine::ob::ErrorCode::None);
  ASSERT_EQ(book.AddOrder(2, 1, 100, 2, Side::Sell, limit, gtc, events),
            engine::ob::ErrorCode::None);
  ASSERT_EQ(book.AddOrder(3, 1, 100, 2, Side::Sell, limit, gtc, events),
            engine::ob::ErrorCode::None);
  ASSERT_EQ(book.AddOrder(4, 2, 101, 5, Side::Buy, limit, gtc, events),
            engine::ob::ErrorCode::None);

  ASSERT_EQ(events.trades.size(), 3);
  EXPECT_EQ(events.trades[0].GetMakerOrderID(), 2);
  EXPECT_EQ(events.trades[1].GetMakerOrderID(), 3);
  EXPECT_EQ(events.trades[2].GetMakerOrderID(), 1);
  EXPECT_EQ(events.trades[2].GetQuantity(), 1);
  book.VerifyIntegrity();
}

TEST(OrderBook, CancelChecksOwnerAndReleasesCapacity) {
  Book book({}, 1);
  Events events;
  ASSERT_EQ(book.AddOrder(1, 10, 100, 2, Side::Buy, limit, gtc, events),
            engine::ob::ErrorCode::None);
  EXPECT_EQ(book.AddOrder(2, 20, 99, 1, Side::Buy, limit, gtc, events),
            engine::ob::ErrorCode::BookAtCapacity);
  EXPECT_EQ(book.CancelOrder(1, 99, Side::Buy, events),
            engine::ob::ErrorCode::InvalidIDMatch);
  EXPECT_EQ(book.CancelOrder(1, 10, Side::Buy, events),
            engine::ob::ErrorCode::None);
  EXPECT_EQ(book.AddOrder(2, 20, 99, 1, Side::Buy, limit, gtc, events),
            engine::ob::ErrorCode::None);
  book.VerifyIntegrity();
}

TEST(OrderBook, ModifyCanRepriceAndMatch) {
  Book book({}, 4);
  Events events;
  ASSERT_EQ(book.AddOrder(1, 10, 105, 4, Side::Sell, limit, gtc, events),
            engine::ob::ErrorCode::None);
  ASSERT_EQ(book.AddOrder(2, 20, 100, 4, Side::Buy, limit, gtc, events),
            engine::ob::ErrorCode::None);

  EXPECT_EQ(book.ModifyOrder(2, 20, 105, 4, events),
            engine::ob::ErrorCode::None);
  ASSERT_EQ(events.trades.size(), 1);
  EXPECT_EQ(events.trades[0].GetQuantity(), 4);
  EXPECT_EQ(book.BidOrderCount(), 0);
  EXPECT_EQ(book.AskOrderCount(), 0);
  book.VerifyIntegrity();
}

TEST(TradingEngine, ConstructsAcceptsCommandAndShutsDown) {
  engine::TradingEngine<engine::matching::PriceTimePriority, 8, 8, 8> engine;
  engine::Command command{
      .type = engine::CommandType::NewOrder,
      .payload = {.new_order = {.price = 100,
                                .qty = 1,
                                .side = Side::Buy,
                                .type = limit,
                                .tif = gtc}}};
  EXPECT_TRUE(engine.try_submit(42, command));
}
} // namespace
