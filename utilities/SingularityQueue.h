#pragma once
#include <atomic>
#include <array>
#include <cstddef>

// Lock-free, wait-free SPSC ring buffer.
// Constraints:
//   - Exactly ONE producer thread calls push()
//   - Exactly ONE consumer thread calls pop()
//   - Capacity must be a power of 2
//   - No dynamic allocation after construction
template<typename T, std::size_t Capacity>
class SingularityQueue
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");
    static constexpr std::size_t Mask = Capacity - 1;

public:
    SingularityQueue() = default;

    // Producer thread only. Returns false if the queue is full.
    bool push(const T& item)
    {
        const std::size_t head = _head.load(std::memory_order_relaxed);
        const std::size_t next = (head + 1) & Mask;
        if (next == _tail.load(std::memory_order_acquire))
            return false;
        _buffer[head] = item;
        _head.store(next, std::memory_order_release);
        return true;
    }

    // Consumer thread only. Returns false if the queue is empty.
    bool pop(T& item)
    {
        const std::size_t tail = _tail.load(std::memory_order_relaxed);
        if (tail == _head.load(std::memory_order_acquire))
            return false;
        item = _buffer[tail];
        _tail.store((tail + 1) & Mask, std::memory_order_release);
        return true;
    }

    bool empty() const noexcept
    {
        return _tail.load(std::memory_order_acquire) == _head.load(std::memory_order_acquire);
    }

private:
    std::array<T, Capacity> _buffer{};

    // Each index on its own cache line to prevent false sharing between threads.
    alignas(64) std::atomic<std::size_t> _head{0};
    alignas(64) std::atomic<std::size_t> _tail{0};
};
