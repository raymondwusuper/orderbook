#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <atomic>
#include <cstdint>
#include "orderbook.hpp"
#include "mpsc_queue.hpp"
#include "spsc_queue.hpp"
#include "events.hpp"

enum class MsgType : uint8_t { Add, Cancel };

struct EngineMsg {
    MsgType type;
    Order order;
    Id cancelId;

    static EngineMsg Add(const Order& o);
    static EngineMsg Cancel(Id id);
};

class MatchingEngine {
public:
    MatchingEngine(MpscQueue<EngineMsg>& inbound, SpscQueue<Event>& outbound);

    void run();
    void stop();

    Orderbook& book() { return book_; }

private:
    MpscQueue<EngineMsg>& inbound_;
    SpscQueue<Event>& outbound_;
    std::atomic<bool> running_;
    Orderbook book_;
    Trades trade_buf_; 
};

#endif // ENGINE_HPP