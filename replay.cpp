#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>

#include "engine.hpp"
#include "stats.hpp"

std::vector<EngineMsg> generate_events(std::size_t n) {
    std::vector<EngineMsg> events;
    events.reserve(n + n / 5);
    for (size_t i = 0; i < n; ++i) {
        Id id = i + 1;
        bool isBuy = i % 2 == 0;
        Price px = 100 + static_cast<Price>(i % 10);
        Quantity qty = 10;
        Order o{id, px, isBuy, qty};
        events.push_back(EngineMsg::Add(o));
        if (i > 5 && (i % 7 == 0)) {
            Id cancelId = id - 5;
            events.push_back(EngineMsg::Cancel(cancelId));
        }
    }
    return events;
}

int main() {
    constexpr size_t INBOUND_CAPACITY  = 1u << 16;
    constexpr size_t OUTBOUND_CAPACITY = 1u << 18; 

    MpscQueue<EngineMsg> inbound(INBOUND_CAPACITY);
    SpscQueue<Event>     outbound(OUTBOUND_CAPACITY);

    const double tsc_ghz = calibrate_tsc_ghz();
    std::cout << "TSC frequency: " << tsc_ghz << " ticks/ns\n";

    MatchingEngine engine(inbound, outbound);
    StatsConsumer  stats(outbound, tsc_ghz);

    std::thread engineThread([&] { engine.run(); });
    std::thread statsThread ([&] { stats.run();  });

    const size_t N_EVENTS = 1'000'000;
    auto events = generate_events(N_EVENTS);

    const unsigned hw = std::thread::hardware_concurrency();
    const size_t N_THREADS = (hw > 2) ? (hw - 2) : 1;
    std::vector<std::thread> producers;
    producers.reserve(N_THREADS);
    size_t chunk = (events.size() + N_THREADS - 1) / N_THREADS;

    auto t0 = std::chrono::steady_clock::now();
    for (size_t p = 0; p < N_THREADS; ++p) {
        size_t start = p * chunk;
        size_t end = std::min(events.size(), start + chunk);
        if (start >= end) break;
        producers.emplace_back([start, end, &inbound, &events] {
            for (size_t i = start; i < end; ++i) {
                const auto& msg = events[i];
                while (!inbound.push(msg)) std::this_thread::yield();
            }
        });
    }
    for (auto& t : producers) t.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    engine.stop();
    engineThread.join();

    stats.stop();
    statsThread.join();

    auto t1 = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(t1 - t0).count();
    size_t sent = events.size();
    double mps = sent / seconds;
    std::cout << "Replayed " << sent << " messages in " << seconds
              << "s (" << mps << " msg/sec)\n";

    stats.report();
    return 0;
}