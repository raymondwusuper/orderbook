#include <vector>
#include <thread>
#include <chrono>
#include <iostream>

#include "engine.hpp"

int main() {
    MpscQueue<EngineMsg> q(1024);
    MatchingEngine engine(q);

    std::thread engineThread([&] {
        engine.run();
    });

    const size_t N_THREADS = 4;
    std::vector<std::thread> producers;
    producers.reserve(N_THREADS);

    for (size_t p = 0; p < N_THREADS; ++p) {
        producers.emplace_back([p, &q] {
            for (int i = 0; i < 10; ++i) {
                Id id = p * 1000 + i;
                Order o{id, 100 + (i % 3), (i % 2) == 0, 10};
                while (!q.push(EngineMsg::Add(o))) {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producers) t.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    engine.stop();
    engineThread.join();

    return 0;
}

