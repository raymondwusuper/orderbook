#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP

#include <cstddef>
#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <functional>

using Id = size_t;
using Price = long;
using Quantity = int;

class Order {
public:
    Order() = default;
    Order(Id orderId, Price level, bool isBuy, Quantity quantity)
        : orderId_(orderId), level_(level), isBuy_(isBuy), quantity_(quantity) {}

    Id OrderId()  const { return orderId_; }
    Id id()       const { return orderId_; }
    Price level() const { return level_; }
    bool isBuy()  const { return isBuy_; }
    Quantity qty() const { return quantity_; }

    void consume(Quantity q) { quantity_ -= q; }
    bool done() const { return quantity_ <= 0; }

private:
    Id orderId_{};
    Price level_{};
    bool isBuy_{};
    Quantity quantity_{};
};

using Orders = std::vector<Order>;

struct Trade {
    Id OrderIdA;
    Id OrderIdB;
    Id AggressorOrderId;
    bool AggressorIsBuy;
    Price Level;
    Quantity Size;
};

using Trades = std::vector<Trade>;
using LevelQ = std::list<Order>;

class Orderbook {
public:
    Trades AddOrder(const Order& order);
    void CancelOrder(Id orderId);

private:
    std::map<Price, LevelQ, std::greater<Price>> bids;
    std::map<Price, LevelQ> asks;

    struct Locator {
        bool isBuy;
        Price price;
        LevelQ::iterator level;
    };
    std::unordered_map<Id, Locator> loc;
};

#endif // ORDERBOOK_HPP

