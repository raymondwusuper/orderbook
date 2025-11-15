#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <atomic>
#include <cassert>
#include <cstdint>

#include "orderbook.hpp"
#include "mpsc_queue.hpp"

enum class MsgType : uint8_t {Add, Cancel};

struct EngineMsg {
    MsgType type;
    Order order;
    Id cancelId;
    
    static EngineMsg Add(const Order& o) {
        EngineMsg m;
        m.type = MsgType::Add;
        m.order = o;
        m.cancelId = 0;
        return m;
    }

    static EngineMsg Cancel(Id id) {
        EngineMsg m;
        m.type = MsgType::Cancel;
        m.cancelId = id;
        return m;
    }
};

class MatchingEngine {
public:
    MatchingEngine(MpscQueue<EngineMsg>& inbound) : inbound_(inbound), running_(true) {}

    void run() {
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
                } else book_.CancelOrder(msg.cancelId);
            } else std::this_thread::yield();
        }
    }

    void stop() {
        running_.store(false, std::memory_order_relaxed);
    }

    Orderbook& book() {return book_;}

private:
    MpscQueue<EngineMsg>& inbound_;
    std::atomic<bool> running_;
    Orderbook book_;
};

int main() {
    MpscQueue<EngineMsg> q(1024);
    MatchingEngine engine(q);
    std::thread engineThread([&]{engine.run();});
    const size_t N_THREADS = 4;
    std::vector<std::thread> producers; producers.reserve(N_THREADS);
    for (int p = 0; p < N_THREADS; ++p) {
        producers.emplace_back([p, &q]{
                    for(int i = 0; i < 10; ++i) {
                        Id id = p * 1000 + i;
                        Order o{id, 100 + (i % 3), (i % 2) == 0, 10};
                        while (!q.push(EngineMsg::Add(o))) std::this_thread::yield();
                    }
                }
            );
    }
    for (auto& t : producers) t.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    engine.stop();
    engineThread.join();
    return 0;

}
