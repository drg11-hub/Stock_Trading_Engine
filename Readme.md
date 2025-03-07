# Real-Time Stock Trading Engine

## Overview
This is a real-time stock trading engine implemented in **C++** that efficiently matches **Buy** and **Sell** orders for stocks. The engine supports **1,024 tickers** and ensures concurrent order processing while maintaining a lock-free structure using **atomic spinlocks**.

## Features
- **Custom Priority Queue**: Uses a linked list-based priority queue (MaxHeap for Buys, MinHeap for Sells) to maintain order price priority.
- **Lock-Free Order Matching**: Implements **atomic spinlocks** to manage concurrent access safely.
- **Efficient Order Matching (O(n))**: Ensures efficient trade execution without using built-in dictionaries/maps.
- **Multi-Threaded Simulation**: Simulates stock transactions using multiple threads to mimic real-world market activity.

## Installation & Compilation
### Prerequisites
- **C++ Compiler** (`g++` recommended)
- **C++17 or later**

### Steps to Compile and Run
1. **Clone the repository**:
   ```sh
   git clone <repository_url>
   cd stock-trading-engine
   ```

2. **Compile the code**:
   ```sh
   g++ -std=c++17 -pthread stock_trading_engine.cpp -o stock_engine
   ```

3. **Run the executable**:
   ```sh
   ./stock_engine
   ```

## Code Structure
- **Order Class**: Represents Buy/Sell orders.
- **PriorityQueue Class**: Implements a custom priority queue using a linked list.
- **OrderBook Class**: Manages order matching and execution.
- **simulate_market_activity()**: Generates random orders for simulation.
- **main()**: Runs multiple threads to process stock trades concurrently.

## Example Output
```
Trade Executed: 50 shares of Ticker 123 at $200.50
Trade Executed: 30 shares of Ticker 987 at $150.75
```

## Contributing
Feel free to contribute by submitting pull requests or reporting issues.

---
Developed for **real-time high-performance stock trading.** 