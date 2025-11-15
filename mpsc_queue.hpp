#ifndef MPSC_QUEUE_HPP
#define MPSC_QUEUE_HPP
#include <atomic>
#include <vector>
#include <cassert>

template <typename T>
class MpscQueue{
public:
    explicit MpscQueue(size_t capacity) : capacity_(capacity), mask_(capacity - 1), data_(capacity) {
        assert((capacity & mask_) == 0 && "capacity must be a power of 2");
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    bool push(const T& value) {
        while (true) {
            size_t head = head_.load(std::memory_order_relaxed);
            size_t tail = tail_.load(std::memory_order_acquire);
            if (head - tail >= capacity_) continue; //spin when full
            if (head_.compare_exchange_weak(head, head + 1, std::memory_order_acq_rel, std::memory_order_relaxed)) {
                data_[head & mask_] = value;
                return true;
            }
        }
    }

    bool pop(T& out) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_acquire);
        if (tail == head) return false; //exit when empty
        out = data_[tail & mask_];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

private:
    const size_t capacity_;
    const size_t mask_;
    std::vector<T> data_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};
#endif //MPSC_QUEUE_HPP
