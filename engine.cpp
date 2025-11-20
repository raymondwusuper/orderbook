#include "engine.hpp"

#include <iostream>
#include <thread>

EngineMsg EngineMsg::Add(const Order& o) {
    EngineMsg m;
    m.type = MsgType::Add;
    m.order = o;
    m.cancelId = 0;
    return m;
}

EngineMsg EngineMsg::Cancel(Id id) {
    EngineMsg m;
    m.type = MsgType::Cancel;
    m.cancelId = id;
    return m;
}

MatchingEngine::MatchingEngine(MpscQueue<EngineMsg>& inbound)
    : inbound_(inbound), running_(true) {}

void MatchingEngine::run() {
    EngineMsg msg;
    while (running_.load(std::memory_order_relaxed)) {
        if (inbound_.pop(msg)) {
            if (msg.type == MsgType::Add) {
                Trades trades = book_.AddOrder(msg.order);
                for (const auto& t : trades) {
                    std::cout << "TRADE "
                              << "A=" << t.OrderIdA
                              << " B=" << t.OrderIdB
                              << " Px=" << t.Level
                              << " Sz=" << t.Size
                              << "\n";
                }
            } else {
                book_.CancelOrder(msg.cancelId);
            }
        } else {
            std::this_thread::yield();
        }
    }
}

void MatchingEngine::stop() {
    running_.store(false, std::memory_order_relaxed);
}

