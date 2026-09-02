#pragma once

#include "Command.hpp"
#include "Data/MutexQueue.hpp"
#include "Data/SPSC_Queue.hpp"
#include "Engine/Event.hpp"
#include "Engine/MatchingPolicy.hpp"
#include "EventDistributor.hpp"
#include "EventEmitter.hpp"
#include "OrderBook.hpp"
#include "PriceTimePriority.hpp"

#include <stop_token>
#include <thread>

namespace Hermes::engine {
inline constexpr std::size_t MAX_COMMANDS = 1ULL << 16, MAX_ORDERS = 1ULL << 16,
                             MAX_EVENTS = 1ULL << 16;

template <
    matching::MatchingPolicy Policy, std::size_t CommandCapacity = MAX_COMMANDS,
    std::size_t EventCapacity = MAX_EVENTS, std::size_t MaxOrders = MAX_ORDERS>
class TradingEngine {
public:
  using InputQueue = data::MutexQueue<CommandEnvelope>;
  using CommandQueue = data::SPSC_Queue<CommandEnvelope, CommandCapacity>;

  using EventQueue = data::SPSC_Queue<Event, EventCapacity>;
  using EventEmitter = events::EventEmitter<EventQueue>;
  using OrderBook = ob::OrderBook<Policy>;

  TradingEngine()
      : m_OrderBook(m_Policy, MaxOrders), m_EventEmitter(m_EventQueue),
        m_EventThread([this](std::stop_token st) { RunEvents(st); }),
        m_SequencerThread([this](std::stop_token st) { RunSequencer(st); }),
        m_OrderBookThread([this](std::stop_token st) { RunBook(st); }) {}

  ~TradingEngine() noexcept {
    m_SequencerThread.request_stop();
    m_SequencerThread.join();
    m_OrderBookThread.request_stop();
    m_OrderBookThread.join();
    m_EventThread.request_stop();
    m_EventThread.join();
  }

  TradingEngine(const TradingEngine &) = delete;
  TradingEngine &operator=(const TradingEngine &) = delete;
  TradingEngine(TradingEngine &&) = delete;
  TradingEngine &operator=(TradingEngine &&) = delete;

  [[nodiscard]] bool try_submit(ClientID clientID, Command command) {
    return m_InputQueue.push(CommandEnvelope{
        .clientID = clientID, .cmd = std::move(command), .seq = 0});
  }

private:
  // Clients -> Input Queue -> Sequencer -> Validation -> Command Queue -> Order
  // Book -> Event Distributor
  InputQueue m_InputQueue;
  CommandQueue m_CommandQueue;

  EventQueue m_EventQueue;
  EventDistributor m_EventDistributor;

  Policy m_Policy;
  OrderBook m_OrderBook;

  SequenceID m_CommandSeq{1};
  OrderID m_OrderIDCounter{1};
  EventEmitter m_EventEmitter;

  std::jthread m_EventThread, m_SequencerThread, m_OrderBookThread;

  bool Validate(const Command &cmd) const noexcept {
    switch (cmd.type) {
    case CommandType::NewOrder:
      return cmd.payload.new_order.qty > 0 &&
             cmd.payload.new_order.type != OrderType::Stop &&
             (cmd.payload.new_order.type == OrderType::Market ||
              cmd.payload.new_order.price > 0);
    case CommandType::CancelOrder:
      return cmd.payload.cancel_order.orderID != 0;
    case CommandType::ModifyOrder:
      return cmd.payload.modify_order.orderID != 0 &&
             cmd.payload.modify_order.price > 0 &&
             cmd.payload.modify_order.quantity > 0;
    }
    return false;
  }

  void RunSequencer(std::stop_token st) noexcept {
    std::optional<CommandEnvelope> env;
    while (!st.stop_requested()) {
      env = m_InputQueue.try_pop();

      if (!env.has_value()) {
        std::this_thread::yield();
        continue;
      }

      env.value().seq = m_CommandSeq++;

      // NOTE: Don't call event distributor here. Instead validation failures
      // travel through the same serialized command -> OrderBook -> event
      // pipeline

      env.value().rejected = !Validate(env.value().cmd);

      while (!m_CommandQueue.try_push(std::move(env.value())))
        std::this_thread::yield();
    }

    while ((env = m_InputQueue.try_pop()).has_value()) {
      env->seq = m_CommandSeq++;
      env->rejected = !Validate(env->cmd);
      while (!m_CommandQueue.try_push(std::move(*env)))
        std::this_thread::yield();
    }
  }

  void RunBook(std::stop_token st) noexcept {
    CommandEnvelope env;
    while (!st.stop_requested())
      if (m_CommandQueue.try_pop(env))
        Process(env);

    while (m_CommandQueue.try_pop(env))
      Process(env);
  }

  void RunEvents(std::stop_token st) noexcept {
    Event event;
    while (!st.stop_requested()) {
      if (m_EventQueue.try_pop(event))
        m_EventDistributor.Publish(event);
      else
        std::this_thread::yield();
    }

    while (m_EventQueue.try_pop(event))
      m_EventDistributor.Publish(event);
  }

  void Process(const CommandEnvelope &env) noexcept {
    if (env.rejected) {
      m_EventEmitter.OrderRejected(env.clientID, env.cmd.GetOrderID());
      return;
    }

    using enum CommandType;
    switch (env.cmd.type) {
    case NewOrder:
      ProcessNewOrder(env);
      break;
    case CancelOrder:
      ProcessCancelOrder(env);
      break;
    case ModifyOrder:
      ProcessModifyOrder(env);
      break;
    default:
      HERMES_ASSERT(false, "Unkown command type");
      break;
    }
  }

  void ProcessNewOrder(const CommandEnvelope &env) noexcept {
    const auto &cmd = env.cmd.payload.new_order;

    const auto res = m_OrderBook.AddOrder(m_OrderIDCounter++, env.clientID,
                                          cmd.price, cmd.qty, cmd.side,
                                          cmd.type, cmd.tif, m_EventEmitter);

    if (res != ob::ErrorCode::None) {
      // TODO: Convert Order book errors into private rejection events
    }
  }

  void ProcessCancelOrder(const CommandEnvelope &env) noexcept {
    const auto &cmd = env.cmd.payload.cancel_order;

    const auto res = m_OrderBook.CancelOrder(cmd.orderID, env.clientID,
                                             cmd.side, m_EventEmitter);

    if (res != ob::ErrorCode::None) {
      // TODO: Convert Order book errors into private rejection events
    }
  }

  void ProcessModifyOrder(const CommandEnvelope &env) noexcept {
    const auto &cmd = env.cmd.payload.modify_order;

    const auto res = m_OrderBook.ModifyOrder(
        cmd.orderID, env.clientID, cmd.price, cmd.quantity, m_EventEmitter);

    if (res != ob::ErrorCode::None) {
      // TODO: Convert Order book errors into private rejection events
    }
  }
};
} // namespace Hermes::engine
