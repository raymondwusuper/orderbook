#ifndef STATS_HPP
#define STATS_HPP

#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cinttypes>
#include <cmath>

#include "events.hpp"
#include "spsc_queue.hpp"

class LogHistogram {
public:
    static constexpr int kBuckets = 64;

    void record(uint64_t v) {
        ++total_;
        sum_ += v;
        if (v > max_) max_ = v;
        int b = (v == 0) ? 0 : (64 - __builtin_clzll(v));
        if (b >= kBuckets) b = kBuckets - 1;
        ++counts_[b];
    }

    uint64_t count() const { return total_; }
    uint64_t max()   const { return max_; }
    double   mean()  const { return total_ ? double(sum_) / total_ : 0.0; }

    uint64_t percentile(double p) const {
        if (total_ == 0) return 0;
        uint64_t target = uint64_t(std::ceil(p / 100.0 * total_));
        uint64_t acc = 0;
        for (int i = 0; i < kBuckets; ++i) {
            acc += counts_[i];
            if (acc >= target) {
                return (i == 0) ? 0 : (uint64_t(1) << (i - 1));
            }
        }
        return max_;
    }

private:
    uint64_t counts_[kBuckets]{};
    uint64_t total_{0};
    uint64_t sum_{0};
    uint64_t max_{0};
};

class StatsConsumer {
public:
    StatsConsumer(SpscQueue<Event>& q, double tsc_ghz)
        : q_(q), tsc_ghz_(tsc_ghz), running_(true) {}

    void stop() { running_.store(false, std::memory_order_relaxed); }

    void run() {
        Event e;
        while (running_.load(std::memory_order_relaxed)) {
            if (!q_.pop(e)) {
                continue;
            }
            consume(e);
        }
        while (q_.pop(e)) consume(e);
    }

    void report() const {
        printf("\n=== Stats Report ===\n");
        printf("Events:   %" PRIu64 " accepts, %" PRIu64 " trades, %" PRIu64 " cancels\n",
               accepts_, trades_, cancels_);
        printf("Volume:   %" PRIu64 " units across trades\n", volume_);
        printf("Batches:  %" PRIu64 " (avg size = %.0f events)\n",
               batches_,
               batches_ ? double(total_events_) / double(batches_) : 0.0);
        if (batches_ == 0) return;

        const double mean_match_ns =
            double(total_work_ticks_) / double(total_events_) / tsc_ghz_;
        printf("Mean match latency: %.1f ns\n", mean_match_ns);

        printf("\nBatch-level throughput (msg/s)\n");
        printf("p50 batch ns/event: %" PRIu64 "\n", batch_ns_hist_.percentile(50.0));
        printf("p99 batch ns/event: %" PRIu64 "\n", batch_ns_hist_.percentile(99.0));
        printf("max batch ns/event: %" PRIu64 "\n", batch_ns_hist_.max());
    }

private:
    void consume(const Event& e) {
        switch (e.type) {
            case EventType::OrderAccepted: ++accepts_; break;
            case EventType::Trade:
                ++trades_;
                volume_ += e.trade.size;
                break;
            case EventType::OrderCanceled: ++cancels_; break;
            case EventType::BatchLatency: {
                ++batches_;
                total_events_     += e.batch.n;
                total_work_ticks_ += e.batch.total_ticks;
                if (e.batch.n > 0) {
                    double per_event_ns =
                        double(e.batch.batch_ticks) / double(e.batch.n) / tsc_ghz_;
                    batch_ns_hist_.record(uint64_t(per_event_ns));
                }
                break;
            }
            default: break;
        }
    }

    SpscQueue<Event>& q_;
    double tsc_ghz_;
    std::atomic<bool> running_;

    LogHistogram batch_ns_hist_;

    uint64_t accepts_{0};
    uint64_t trades_{0};
    uint64_t cancels_{0};
    uint64_t volume_{0};
    uint64_t batches_{0};
    uint64_t total_events_{0};
    uint64_t total_work_ticks_{0};
};

#endif // STATS_HPP