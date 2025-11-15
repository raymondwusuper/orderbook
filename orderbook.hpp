#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP
#include <iostream>
#include <cstddef>
#include <vector>
#include <map>
#include <functional>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <iterator>


using Id = size_t;
using Price = long;
using Quantity = int;


class Order {
public:
   Order(Id orderId, Price level, bool isBuy, Quantity quantity) :
   orderId_(orderId), level_(level), isBuy_(isBuy), quantity_(quantity) {}


   Id OrderId()        const { return orderId_; }
   Id id()        const { return orderId_; }
   Price level()  const { return level_; }
   bool isBuy()   const { return isBuy_; }
   Quantity qty() const { return quantity_; }


   void consume(Quantity q) { quantity_ -= q; }
   bool done() const { return quantity_ <= 0; }


private:
   Id orderId_;
   Price level_;
   bool isBuy_;
   Quantity quantity_;
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
   Trades AddOrder(const Order& order) {
       if (loc.find(order.id()) != loc.end()) return {};
       Order o = order;
       Trades trades;
       if (o.isBuy()) {
           while (!o.done() && !asks.empty()) {
               auto curr = asks.begin();
               Price currlvl = curr->first;
               if (o.level() < currlvl) break;
               auto& queue = curr->second;
               if (queue.empty()) {
                   asks.erase(curr);
                   continue;
               }
               Order& resting = queue.front();
               Quantity trdQt = std::min(o.qty(), resting.qty());
               trades.push_back(Trade{resting.id(), o.id(), o.id(), o.isBuy(), resting.level(), trdQt});
               resting.consume(trdQt);
               o.consume(trdQt);
               if (resting.done()) {
                   loc.erase(resting.id());
                   queue.pop_front();
                   if (queue.empty()) asks.erase(curr);
               }
           }
           if (!o.done()) {
               auto& queue = bids[o.level()];
               queue.push_back(o);
               auto it = std::prev(queue.end());
               loc[o.id()] = Locator{o.isBuy(), o.level(), it};
           }
       } else {
           while (!o.done() && !bids.empty()) {
               auto curr = bids.begin();
               Price currlvl = curr->first;
               if (currlvl < o.level()) break;
               auto& queue = curr->second;
               if (queue.empty()) {
                   bids.erase(curr);
                   continue;
               }
               Order& resting = queue.front();
               Quantity trdQt = std::min(o.qty(), resting.qty());
               trades.push_back(Trade{resting.id(), o.id(), o.id(), o.isBuy(), resting.level(), trdQt});
               resting.consume(trdQt);
               o.consume(trdQt);
               if (resting.done()) {
                   loc.erase(resting.id());
                   queue.pop_front();
                   if (queue.empty()) bids.erase(curr);
               }
           }
           if (!o.done()) {
               auto& queue = asks[o.level()];
               queue.push_back(o);
               auto it = std::prev(queue.end());
               loc[o.id()] = Locator{o.isBuy(), o.level(), it};
           }
       }
       return trades;
   }


   void CancelOrder(Id orderId) {
       auto it = loc.find(orderId);
       if (it == loc.end()) return;
       const auto [isBuy, price, node] = it->second;
       if (isBuy) {
           auto lvl = bids.find(price);
           if (lvl != bids.end()) {
               lvl->second.erase(node);
               if (lvl->second.empty()) bids.erase(lvl);
           }
       } else {
           auto lvl = asks.find(price);
           if (lvl != asks.end()) {
               lvl->second.erase(node);
               if (lvl->second.empty()) asks.erase(lvl);
           }
       }
       loc.erase(it);
   }




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
#endif //ORDERBOOK_HPP
