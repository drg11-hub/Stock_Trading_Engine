#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <mutex>

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

class Node {
public:
    Order order;
    Node* next;

    Node(Order ord) : order(ord), next(nullptr) {}
};

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