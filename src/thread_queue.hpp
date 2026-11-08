#include <array>
#include <atomic>
#include <cstddef>

template <typename T, size_t Capacity>
class SpscRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

private:
    static constexpr size_t BufferMask = Capacity - 1;

    std::array<T, Capacity> buffer_{};

    // alignas(64) prevents false sharing
    alignas(64) std::atomic<size_t> tail_{0};
    alignas(64) std::atomic<size_t> head_{0};

public:
    SpscRingBuffer() = default;

    //returns false if the buffer is full
    bool push(T item) {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);
        const size_t current_head = head_.load(std::memory_order_acquire);


        if (current_tail - current_head >= Capacity) {
            return false;
        }


        buffer_[current_tail & BufferMask] = std::move(item);

        tail_.store(current_tail + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        const size_t current_tail = tail_.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return false;
        }

        item = std::move(buffer_[current_head & BufferMask]);

        head_.store(current_head + 1, std::memory_order_release);
        return true;
    }
};
