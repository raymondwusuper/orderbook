#ifndef MPSC_QUEUE_HPP
#define MPSC_QUEUE_HPP

#include <atomic>
#include <vector>
#include <cassert>
#include <cstddef>
#include <utility>

template <typename T>
class MpscQueue {
public:
    explicit MpscQueue(size_t capacity)
        : capacity_(capacity), mask_(capacity - 1),
          data_(capacity), seq_(capacity) { 
        assert((capacity & mask_) == 0 && "capacity must be a power of 2");
        for (size_t i = 0; i < capacity_; ++i) {
            seq_[i].store(i, std::memory_order_relaxed);
        }
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    bool push(const T& value) {
        size_t pos = head_.load(std::memory_order_relaxed);
        while (true) {
            size_t tail = tail_.load(std::memory_order_acquire);
            if (pos - tail >= capacity_) return false;
            if (head_.compare_exchange_weak(
                    pos, pos + 1,
                    std::memory_order_acq_rel
                    std::memory_order_relaxed)) {
                auto& s = seq_[pos & mask_];
                while (s.load(std::memory_order_acquire) != pos) {}
                data_[pos & mask_] = value;
                s.store(pos + 1, std::memory_order_release);
                return true;
            }
        }
    }

    bool pop(T& out) {
        size_t pos = tail_.load(std::memory_order_relaxed);
        auto& s = seq_[pos & mask_];
        if (s.load(std::memory_order_acquire) != pos + 1) return false;
        out = std::move(data_[pos & mask_]);
        s.store(pos + capacity_, std::memory_order_release);
        tail_.store(pos + 1, std::memory_order_release);
        return true;
    }

private:
    const size_t capacity_;
    const size_t mask_;
    std::vector<T> data_;
    std::vector<std::atomic<size_t>> seq_; 
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};

#endif // MPSC_QUEUE_HPP

