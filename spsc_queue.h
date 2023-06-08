#pragma once

#include <array>
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
        if (is_full()) {
            return false;
        }

        data_[head_] = std::move(value);
        head_ = next(head_);

        return true;
    }

    std::optional<T> try_pop() {
        if (is_empty()) {
            return std::nullopt;
        }

        T value = std::move(data_[tail_]);
        tail_ = next(tail_);

        return value;
    }

  private:
    bool is_empty() const {
        return head_ == tail_;
    }

    bool is_full() const {
        return next(head_) == tail_;
    }

    std::size_t next(std::size_t index) const {
        return (index + 1) % data_.size();
    }
    std::array<T, Capacity> data_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
};
} // namespace spsc