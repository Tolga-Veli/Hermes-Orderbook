#pragma once

#include "Core/FunctionRef.hpp"
#include "Core/Trade.hpp"

#include "MatchingPolicy.hpp"
#include "NodePool.hpp"
#include "OrderBookSide.hpp"
#include "PriceLevel.hpp"

namespace Hermes::engine::matching {
using TradeCallback = core::FunctionRef<void(const core::Trade &)>;

template <MatchingPolicy Policy> class Match {
public:
  Match(Policy policy, TradeID &ID_Counter) noexcept
      : m_Policy(std::move(policy)), m_TradeIDCounter(ID_Counter) {}

  template <Side contraSide>
  void Run(core::Order &incoming, ob::OrderBookSide<contraSide> &book,
           NodePool<ob::OrderNode> &pool, TradeCallback &onTrade) noexcept {
    while (!incoming.isFilled()) {
      ob::PriceLevel *level = book.BestLevel();
      if (level == nullptr || !Crosses(incoming, level->GetPrice()))
        break;

      auto execute = [&](core::Order &taker, ob::OrderNode &maker,
                         Quantity quantity) noexcept {
        Execute(taker, maker, quantity, book, pool, onTrade);
      };
      ExecuteTradeCallback callback{execute};
      MatchingContext context{incoming, *level, callback};
      m_Policy.match(context);
    }
  }

private:
  Policy m_Policy;
  TradeID &m_TradeIDCounter;

  static bool Crosses(const core::Order &incoming, Price levelPrice) noexcept {
    if (incoming.GetOrderType() == OrderType::Market)
      return true;

    return incoming.GetSide() == Side::Buy ? incoming.GetPrice() >= levelPrice
                                           : incoming.GetPrice() <= levelPrice;
  }

  template <Side side>
  void Execute(core::Order &taker, ob::OrderNode &makerNode, Quantity quantity,
               ob::OrderBookSide<side> &contraSide,
               NodePool<ob::OrderNode> &pool, TradeCallback &onTrade) noexcept {
    taker.Fill(quantity);
    makerNode.order.Fill(quantity);
    makerNode.level->ReduceQuantity(quantity);

    const core::Trade trade{++m_TradeIDCounter, makerNode.order.GetOrderID(),
                            taker.GetOrderID(), makerNode.order.GetPrice(),
                            quantity,           taker.GetSide()};

    onTrade(trade);

    if (makerNode.order.isFilled()) {
      contraSide.Remove(&makerNode);
      pool.Release(&makerNode);
    }
  }
};
} // namespace Hermes::engine::matching
