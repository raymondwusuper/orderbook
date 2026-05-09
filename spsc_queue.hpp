#ifndef SPSC_QUEUE_HPP
#define SPSC_QUEUE_HPP

#include <atomic>
#include <vector>
#include <cassert>
#include <cstddef>
#include <new>

constexpr std::size_t kCacheLine = 64;

template <typename T>
class SpscQueue {
public:
    explicit SpscQueue(std::size_t capacity)
        : capacity_(capacity), mask_(capacity - 1), data_(capacity) {
        assert((capacity & mask_) == 0 && "capacity must be a power of 2");
    }

    bool push(const T& v) {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t next = h + 1;
        if (next - cached_tail_ > capacity_) {
            cached_tail_ = tail_.load(std::memory_order_acquire);
            if (next - cached_tail_ > capacity_) return false;
        }
        data_[h & mask_] = v;
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& out) {
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        if (t == cached_head_) {
            cached_head_ = head_.load(std::memory_order_acquire);
            if (t == cached_head_) return false;
        }
        out = data_[t & mask_];
        tail_.store(t + 1, std::memory_order_release);
        return true;
    }

private:
    const std::size_t capacity_;
    const std::size_t mask_;
    std::vector<T> data_;

    alignas(kCacheLine) std::atomic<std::size_t> head_{0};
    std::size_t cached_tail_{0};

    alignas(kCacheLine) std::atomic<std::size_t> tail_{0};
    std::size_t cached_head_{0};
};

#endif // SPSC_QUEUE_HPP