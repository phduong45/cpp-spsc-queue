#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <utility>

namespace spsc {
inline constexpr std::size_t kCacheLineSize = 64;

template <typename T, std::size_t Capacity>
class SpscQueue {
    static constexpr bool is_power_of_two(std::size_t value) {
        return value > 0 && (value & (value - 1)) == 0;
    }

    static_assert(is_power_of_two(Capacity),
                  "SpscQueue capacity must be a power of two");

  public:
    static constexpr std::size_t capacity() {
        return Capacity;
    }

    bool try_push(T value) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t tail = tail_.load(std::memory_order_acquire);
        if (head - tail == Capacity) {
            return false;
        }

        data_[index(head)] = std::move(value);
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    std::optional<T> try_pop() {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (head == tail) {
            return std::nullopt;
        }

        T value = std::move(data_[index(tail)]);
        tail_.store(tail + 1, std::memory_order_release);
        return value;
    }

  private:
    std::size_t index(std::size_t counter) const {
        return counter & (Capacity - 1);
    }
    std::array<T, Capacity> data_{};
    alignas(kCacheLineSize) std::atomic<std::size_t> head_{0};
    alignas(kCacheLineSize) std::atomic<std::size_t> tail_{0};
};
} // namespace spsc
