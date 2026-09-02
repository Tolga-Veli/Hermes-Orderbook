# Trading-Engine
A fast, **C++23** trading engine implementing **price–time priority matching**, with a simple terminal-based visualization and order flow simulation with random orders.
![Live orderbook simulation](assets/orderbook.gif)


## Overview
The trading engine implements the **core mechanics of a limit orderbook**.  
The project dives into **data structure design, algorithmic complexity, and performance trade-offs** without any external dependencies.

## Features
- Price–time priority order matching
- Live terminal visualization of the orderbook


## Matching Rules & Guarantees
The orderbook enforces the following guarantees:

- **Price priority:** better prices are matched first
- **Time priority:** FIFO execution within the same price level
- **Determinism:** identical inputs produce identical outputs
- **Correct quantity handling:** no negative quantities; fully filled orders are removed


## Data Structures & Complexity
The implementation is performance oriented.

- Each price is a key in a Red-Black tree from the STL that holds a linked list (`std::pmr::map<Price, std::pmr::list<Order>>`)
- The linked list is used with iterators stored in a hash table for O(1) removal of orders
- Matching operations prioritize best bid / best ask

**Asymptotic behavior:**
- Order insertion: *O(log n)*
- Order cancellation: *O(1)*
- Best price query: *O(1)*


## Architecture Overview
High-level components:

- `Order` — represents individual orders
- `Trade` — represents each completed interaction between two orders by the matching algorithm
- `OrderBook` — maintains bid/ask price levels
- `List` — templated custom linked list implementation for maximum efficiency (STL compatible)
- `MatchingEngine` — executes matching logic
- `Renderer` — terminal UI for visualization

## Prerequisites
- **C++ Compiler** with C++23 support:
- **CMake** 3.20 or higher
- **Git** (for cloning and submodules)

## How to build the project
This project uses **Git Submodules** and **CMake**. To clone the repository:
```bash
git clone --recursive https://github.com/Tolga-Veli/Hermes-Orderbook.git
```

If you have already cloned the repo without `--recursive`, initialize the submodules with this command:
```bash
git submodule update --init --recursive
```

And to build it via the command line, run the following commands from the project root:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
