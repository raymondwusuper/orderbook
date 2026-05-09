#include "engine.hpp"
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

MatchingEngine::MatchingEngine(MpscQueue<EngineMsg>& inbound,
                               SpscQueue<Event>& outbound)
    : inbound_(inbound), outbound_(outbound), running_(true) {
    trade_buf_.reserve(64);
}

static inline void emit(SpscQueue<Event>& q, const Event& e) {
    while (!q.push(e)) std::this_thread::yield();
}

static constexpr uint32_t kBatchSize = 1024;

void MatchingEngine::run() {
    EngineMsg msg;

    uint32_t batch_count    = 0;
    uint64_t batch_work_sum = 0; 
    uint64_t batch_t_start  = rdtsc_now();

    while (running_.load(std::memory_order_relaxed)) {
        if (!inbound_.pop(msg)) {
            std::this_thread::yield();
            continue;
        }

        const uint64_t t0 = rdtsc_now();

        if (msg.type == MsgType::Add) {
            Trades trades = book_.AddOrder(msg.order);
            const uint64_t t1 = rdtsc_now();
            batch_work_sum += (t1 - t0);

            Event acc{};
            acc.type = EventType::OrderAccepted;
            acc.accept = {msg.order.id(), msg.order.level(),
                          msg.order.qty(), msg.order.isBuy()};
            emit(outbound_, acc);

            for (const auto& t : trades) {
                Event e{};
                e.type = EventType::Trade;
                e.trade = {t.OrderIdA, t.OrderIdB, t.Level, t.Size,
                           t.AggressorIsBuy};
                emit(outbound_, e);
            }
        } else {
            book_.CancelOrder(msg.cancelId);
            const uint64_t t1 = rdtsc_now();
            batch_work_sum += (t1 - t0);

            Event e{};
            e.type = EventType::OrderCanceled;
            e.cancel = {msg.cancelId, true};
            emit(outbound_, e);
        }

        if (++batch_count >= kBatchSize) {
            const uint64_t batch_t_end = rdtsc_now();
            Event b{};
            b.type = EventType::BatchLatency;
            b.batch = {batch_count, batch_work_sum, batch_t_end - batch_t_start};
            emit(outbound_, b);

            batch_count    = 0;
            batch_work_sum = 0;
            batch_t_start  = batch_t_end;
        }
    }

    if (batch_count > 0) {
        const uint64_t batch_t_end = rdtsc_now();
        Event b{};
        b.type = EventType::BatchLatency;
        b.batch = {batch_count, batch_work_sum, batch_t_end - batch_t_start};
        emit(outbound_, b);
    }
}

void MatchingEngine::stop() {
    running_.store(false, std::memory_order_relaxed);
}