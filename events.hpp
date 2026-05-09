#ifndef EVENTS_HPP
#define EVENTS_HPP

#include <cstdint>
#include <chrono>
#include <thread>

#include "orderbook.hpp"

#if defined(__x86_64__) || defined(_M_X64)
  #include <x86intrin.h>
  inline uint64_t rdtsc_now() {
      _mm_lfence();
      uint64_t t = __rdtsc();
      _mm_lfence();
      return t;
  }
#elif defined(__aarch64__)
  inline uint64_t rdtsc_now() {
      asm volatile("isb" ::: "memory");
      uint64_t t;
      asm volatile("mrs %0, cntvct_el0" : "=r"(t));
      return t;
  }

  inline uint64_t arm_timer_freq_hz() {
      uint64_t f;
      asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
      return f;
  }
#else
  #error "Unsupported architecture for rdtsc_now()"
#endif

inline double calibrate_ticks_per_ns() {
#if defined(__aarch64__)
    return double(arm_timer_freq_hz()) / 1e9;
#else
    using clk = std::chrono::steady_clock;
    auto wall0 = clk::now();
    uint64_t tsc0 = rdtsc_now();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint64_t tsc1 = rdtsc_now();
    auto wall1 = clk::now();
    double ns = std::chrono::duration<double, std::nano>(wall1 - wall0).count();
    return double(tsc1 - tsc0) / ns;
#endif
}

inline double calibrate_tsc_ghz() { return calibrate_ticks_per_ns(); }

enum class EventType : uint8_t {
    Trade,
    OrderAccepted,
    OrderCanceled,
    OrderRejected,
    BatchLatency,  
};

struct TradeEvent {
    Id maker;
    Id taker;
    Price level;
    Quantity size;
    bool aggressorIsBuy;
};

struct AcceptEvent {
    Id id;
    Price level;
    Quantity size;
    bool isBuy;
};

struct CancelEvent {
    Id id;
    bool found;  
};

struct BatchLatencyEvent {
    uint32_t n;         
    uint64_t total_ticks;   
    uint64_t batch_ticks;   
};

struct Event {
    EventType type;
    union {
        TradeEvent        trade;
        AcceptEvent       accept;
        CancelEvent       cancel;
        BatchLatencyEvent batch;
    };
};

#endif // EVENTS_HPP