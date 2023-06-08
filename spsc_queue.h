#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <utility>

namespace spsc {

template <typename T, std::size_t Capacity>
class SpscQueue {
    static_assert(Capacity > 0, "SpscQueue capacity must be greater than 0");

  public:
    static constexpr std::size_t capacity() {
        return Capacity;
    }

    bool try_push(T value) {
        const std::size_t head = head_.load();
        const std::size_t tail = tail_.load();
        if (next(head) == tail) {
            return false;
        }

        data_[head] = std::move(value);
        head_.store(next(head));
        return true;
    }

    std::optional<T> try_pop() {
        const std::size_t head = head_.load();
        const std::size_t tail = tail_.load();
        if (head == tail) {
            return std::nullopt;
        }

        T value = std::move(data_[tail]);
        tail_.store(next(tail));
        return value;
    }

  private:
    std::size_t next(std::size_t index) const {
        return (index + 1) % data_.size();
    }
    std::array<T, Capacity> data_{};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};
} // namespace spsc