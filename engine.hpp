#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <atomic>
#include <cstdint>
#include "orderbook.hpp"
#include "mpsc_queue.hpp"

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
    explicit MatchingEngine(MpscQueue<EngineMsg>& inbound);

    void run();
    void stop();

    Orderbook& book() { return book_; }

private:
    MpscQueue<EngineMsg>& inbound_;
    std::atomic<bool> running_;
    Orderbook book_;
    std::atomic<size_t> tradeCount_;
};

#endif // ENGINE_HPP

