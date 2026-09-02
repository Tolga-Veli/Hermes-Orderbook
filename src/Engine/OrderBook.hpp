#pragma once

#include "Core/Utils.hpp"
#include "Engine/PriceLevel.hpp"
#include "Match.hpp"
#include "NodePool.hpp"

#include <cstddef>
#include <utility>

namespace Hermes::engine::ob {

enum class ErrorCode {
  None,

  UnknownOrderID,
  DuplicateOrderID,
  InvalidIDMatch,

  BookAtCapacity,
  InvalidQuantity,
};

template <matching::MatchingPolicy Policy> class OrderBook {
public:
  OrderBook(Policy policy, std::size_t maxOrders)
      : m_Pool(maxOrders), m_BuyMatch(policy, m_TradeIDCounter),
        m_SellMatch(std::move(policy), m_TradeIDCounter) {}

  template <class EventEmitter>
  ErrorCode AddOrder(OrderID id, ClientID client, Price price, Quantity qty,
                     Side side, OrderType type, TimeInForce tif,
                     EventEmitter &events) noexcept {
    if (qty == 0)
      return ErrorCode::InvalidQuantity;
    if (m_Bids.Find(id) || m_Asks.Find(id))
      return ErrorCode::DuplicateOrderID;

    OrderNode *node = m_Pool.Acquire(id, client, price, qty, side, type, tif);
    if (node == nullptr)
      return ErrorCode::BookAtCapacity;

    events.OrderAccepted(client, id);

    auto tradeHandler = [&](const core::Trade &trade) noexcept {
      events.Trade(trade);
    };
    matching::TradeCallback onTrade{tradeHandler};

    if (side == Side::Buy)
      m_BuyMatch.Run(node->order, m_Asks, m_Pool, onTrade);
    else
      m_SellMatch.Run(node->order, m_Bids, m_Pool, onTrade);

    const bool restable = node->order.GetRemainingQuantity() > 0 &&
                          tif != TimeInForce::IOC && type == OrderType::Limit;

    // TODO: Dertermine whether this created a new price level or modified an
    // existing level and emit BookAdd / BookModify accordingly
    if (restable) {
      if (side == Side::Buy)
        m_Bids.Insert(node);
      else
        m_Asks.Insert(node);
    } else
      m_Pool.Release(node);

    return ErrorCode::None;
  }

  template <class EventEmitter>
  ErrorCode CancelOrder(OrderID orderID, ClientID clientID, Side side,
                        EventEmitter &events) noexcept {
    OrderNode *node =
        (side == Side::Buy ? m_Bids.Find(orderID) : m_Asks.Find(orderID));

    if (node == nullptr)
      return ErrorCode::UnknownOrderID;

    if (node->order.GetClientID() != clientID)
      return ErrorCode::InvalidIDMatch;

    const Price price = node->order.GetPrice();

    if (side == Side::Buy)
      m_Bids.Remove(node);
    else
      m_Asks.Remove(node);

    m_Pool.Release(node);

    events.OrderCancelled(clientID, orderID);

    // TODO: if removing this order emptied the price level
    // events.BookDelete(side,price) otherwise
    // events.BookModify(side,price,new_quantity);

    return ErrorCode::None;
  }

  template <class EventEmitter>
  ErrorCode ModifyOrder(OrderID orderID, ClientID clientID, Price price,
                        Quantity quantity, EventEmitter &events) noexcept {
    OrderNode *node = m_Bids.Find(orderID);
    Side side = Side::Buy;
    if (node == nullptr) {
      node = m_Asks.Find(orderID);
      side = Side::Sell;
    }

    if (node == nullptr)
      return ErrorCode::UnknownOrderID;
    if (node->order.GetClientID() != clientID)
      return ErrorCode::InvalidIDMatch;

    const Quantity filled = node->order.GetFilledQuantity();
    if (quantity < filled || quantity == 0)
      return ErrorCode::InvalidQuantity;

    const Price oldPrice = node->order.GetPrice();
    const Quantity oldRemaining = node->order.GetRemainingQuantity();
    const bool keepsPriority =
        price == oldPrice && quantity <= node->order.GetInitialQuantity();

    if (keepsPriority) {
      node->order.ModifyOrder(price, quantity);
      const Quantity newRemaining = node->order.GetRemainingQuantity();
      if (newRemaining < oldRemaining)
        node->level->ReduceQuantity(oldRemaining - newRemaining);
      if (node->order.isFilled()) {
        if (side == Side::Buy)
          m_Bids.Remove(node);
        else
          m_Asks.Remove(node);
        m_Pool.Release(node);
      }
      events.OrderModified(clientID, orderID);
      return ErrorCode::None;
    }

    if (side == Side::Buy)
      m_Bids.Remove(node);
    else
      m_Asks.Remove(node);

    node->order.ModifyOrder(price, quantity);
    auto tradeHandler = [&](const core::Trade &trade) noexcept {
      events.Trade(trade);
    };

    matching::TradeCallback onTrade{tradeHandler};
    if (side == Side::Buy)
      m_BuyMatch.Run(node->order, m_Asks, m_Pool, onTrade);
    else
      m_SellMatch.Run(node->order, m_Bids, m_Pool, onTrade);

    if (!node->order.isFilled()) {
      if (side == Side::Buy)
        m_Bids.Insert(node);
      else
        m_Asks.Insert(node);
    } else {
      m_Pool.Release(node);
    }

    events.OrderModified(clientID, orderID);

    return ErrorCode::None;
  }

  std::size_t BidOrderCount() const noexcept { return m_Bids.OrderCount(); }
  std::size_t AskOrderCount() const noexcept { return m_Asks.OrderCount(); }

  void VerifyIntegrity() const noexcept {
    m_Bids.VerifyIntegrity();
    m_Asks.VerifyIntegrity();
  }

private:
  NodePool<OrderNode> m_Pool;

  OrderBookSide<Side::Buy> m_Bids;
  OrderBookSide<Side::Sell> m_Asks;

  TradeID m_TradeIDCounter{};
  matching::Match<Policy> m_BuyMatch, m_SellMatch;
};
} // namespace Hermes::engine::ob
