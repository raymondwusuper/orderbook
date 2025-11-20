#include "orderbook.hpp"
#include <algorithm>

Trades Orderbook::AddOrder(const Order& order) {
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
            trades.push_back(Trade{
                resting.id(),
                o.id(),
                o.id(),
                o.isBuy(),
                resting.level(),
                trdQt
            });

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
            trades.push_back(Trade{
                resting.id(),
                o.id(),
                o.id(),
                o.isBuy(),
                resting.level(),
                trdQt
            });

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

void Orderbook::CancelOrder(Id orderId) {
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

