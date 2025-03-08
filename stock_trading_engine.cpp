#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <mutex>

/*
 * Order class represents a stock market order (Buy or Sell).
 * It contains essential attributes such as order type, ticker symbol,
 * quantity of shares, price per share, and a unique order ID.
 * This class is used as the fundamental unit of stock transactions.
 */

class Order {
public:
    // "Buy" or "Sell"
    std::string order_type;
    int ticker;
    int quantity;
    double price;
    int order_id;
    
    Order(std::string type, int tick, int qty, double prc, int id)
        : order_type(type), ticker(tick), quantity(qty), price(prc), order_id(id) {}
};

/*
 * Node class serves as an element of the priority queue.
 * Each node holds an order and a pointer to the next node,
 * forming a linked list-based priority queue.
 */

class Node {
public:
    Order order;
    Node* next;

    Node(Order ord) : order(ord), next(nullptr) {}
};

/*
 * PriorityQueue implements a sorted linked list acting as a priority queue.
 * It maintains orders in a sorted fashion based on price priority.
 * - Buy orders are stored in a max-heap fashion (higher price = higher priority).
 * - Sell orders are stored in a min-heap fashion (lower price = higher priority).
 * This structure allows O(n) insertion but ensures the highest-priority order
 * is accessible in O(1) time.
 */

class PriorityQueue {
private:
    Node* head;
    bool is_max_heap;

public:
    PriorityQueue(bool max_heap) : head(nullptr), is_max_heap(max_heap) {}

    void insert(Order order) {
        Node* new_node = new Node(order);
        if (!head || (is_max_heap && order.price > head->order.price) || (!is_max_heap && order.price < head->order.price)) {
            new_node->next = head;
            head = new_node;
        } else {
            Node* current = head;
            while (current->next && ((is_max_heap && current->next->order.price > order.price) || (!is_max_heap && current->next->order.price < order.price))) {
                current = current->next;
            }
            new_node->next = current->next;
            current->next = new_node;
        }
    }

    Order pop() {
        if (!head) throw std::runtime_error("Queue is empty");
        Node* temp = head;
        Order ord = head->order;
        head = head->next;
        delete temp;
        return ord;
    }

    Order* peek() {
        return head ? &head->order : nullptr;
    }

    bool is_empty() {
        return head == nullptr;
    }
};

/*
 * OrderBook class manages stock transactions and order matching.
 * - It maintains two priority queues per stock ticker (one for buy orders, one for sell orders).
 * - Uses a lock-free approach with atomic spinlocks to handle concurrent access.
 * - Orders are added to the respective queue and matched in real-time if conditions allow.
 */

class OrderBook {
private:
    static constexpr int max_tickers = 1024;
    std::vector<PriorityQueue> buy_orders;
    std::vector<PriorityQueue> sell_orders;
    std::vector<std::atomic_flag> locks;
    std::atomic<int> order_id_counter;

public:
    OrderBook() : order_id_counter(0), buy_orders(max_tickers, PriorityQueue(true)), sell_orders(max_tickers, PriorityQueue(false)), locks(max_tickers) {}

    void add_order(std::string order_type, int ticker, int quantity, double price) {
        int order_id = order_id_counter.fetch_add(1);
        Order order(order_type, ticker, quantity, price, order_id);
        int index = ticker % max_tickers;
        
        // Spinlock
        while (locks[index].test_and_set(std::memory_order_acquire));
        if (order_type == "Buy") {
            buy_orders[index].insert(order);
        } else {
            sell_orders[index].insert(order);
        }
        locks[index].clear(std::memory_order_release);
        
        match_order(ticker);
    }

    /*
     * Match buy and sell orders for a given ticker.
     * - The best buy order is executed against the best sell order if the price conditions match.
     * - If only part of the order can be fulfilled, the remaining shares are reinserted into the queue.
     */

    void match_order(int ticker) {
        int index = ticker % max_tickers;
        // Spinlock
        while (locks[index].test_and_set(std::memory_order_acquire));
        while (!buy_orders[index].is_empty() && !sell_orders[index].is_empty()) {
            Order* best_buy = buy_orders[index].peek();
            Order* best_sell = sell_orders[index].peek();
            
            if (best_buy->price >= best_sell->price) {
                Order executed_buy = buy_orders[index].pop();
                Order executed_sell = sell_orders[index].pop();
                
                int trade_quantity = std::min(executed_buy.quantity, executed_sell.quantity);
                executed_buy.quantity -= trade_quantity;
                executed_sell.quantity -= trade_quantity;
                
                std::cout << "Trade Executed: " << trade_quantity << " shares of Ticker " << ticker << " at $" << executed_sell.price << std::endl;
                
                if (executed_buy.quantity > 0) {
                    buy_orders[index].insert(executed_buy);
                }
                if (executed_sell.quantity > 0) {
                    sell_orders[index].insert(executed_sell);
                }
            } else {
                break;
            }
        }
        locks[index].clear(std::memory_order_release);
    }
};

OrderBook order_book;

/*
 * Simulates real-time stock transactions with random orders.
 * - Creates buy/sell orders with random prices and quantities.
 * - Runs continuously with multiple threads to mimic real-world order flow.
 */

void simulate_market_activity(int iterations) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ticker_dist(0, 1023);
    std::uniform_int_distribution<> quantity_dist(1, 100);
    std::uniform_real_distribution<> price_dist(10.0, 500.0);
    std::uniform_int_distribution<> type_dist(0, 1);

    for (int i = 0; i < iterations; i++) {
        std::string order_type = type_dist(gen) ? "Buy" : "Sell";
        int ticker = ticker_dist(gen);
        int quantity = quantity_dist(gen);
        double price = price_dist(gen);
        order_book.add_order(order_type, ticker, quantity, price);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

/*
 * Main function launches multiple threads to simulate live trading.
 * - Each thread processes 500 random stock orders.
 * - Ensures concurrent access and execution of trades.
 */

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(simulate_market_activity, 500);
    }
    for (auto& t : threads) {
        t.join();
    }
    return 0;
}