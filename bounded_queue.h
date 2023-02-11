#pragma once

#include <array>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>

namespace spsc {

template <typename T, std::size_t Capacity>
class BoundedQueue {
    static_assert(Capacity > 0, "BoundedQueue capacity must be greater than 0");

  public:
    static constexpr std::size_t capacity() {
        return Capacity;
    }

    bool push(T value) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_full_.wait(lock, [this] { return !full() || closed_; });

            if (closed_) {
                return false;
            }

            push_unchecked(std::move(value));
        }

        not_empty_.notify_one();
        return true;
    }

    bool pop(T& value) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_empty_.wait(lock, [this] { return !empty() || closed_; });

            if (empty() && closed_) {
                return false;
            }

            value = pop_unchecked();
        }

        not_full_.notify_one();
        return true;
    }

    std::optional<T> pop() {
        T value;
        if (!pop(value)) {
            return std::nullopt;
        }

        return value;
    }

    template <typename... Args>
    bool emplace(Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_full_.wait(lock, [this] { return !full() || closed_; });

            if (closed_) {
                return false;
            }

            emplace_unchecked(std::forward<Args>(args)...);
        }

        not_empty_.notify_one();
        return true;
    }

    bool try_push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || full()) {
            return false;
        }

        push_unchecked(std::move(value));
        not_empty_.notify_one();

        return true;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (empty()) {
            return false;
        }

        value = pop_unchecked();
        not_full_.notify_one();

        return true;
    }

    std::optional<T> try_pop() {
        T value;
        if (!try_pop(value)) {
            return std::nullopt;
        }

        return value;
    }

    template <typename... Args>
    bool try_emplace(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || full()) {
            return false;
        }

        emplace_unchecked(std::forward<Args>(args)...);
        not_empty_.notify_one();
        return true;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }

        not_empty_.notify_all();
        not_full_.notify_all();
    }

  private:
    bool empty() const {
        return size_ == 0;
    }

    bool full() const {
        return size_ == data_.size();
    }

    void push_unchecked(T value) {
        data_[head_] = std::move(value);
        head_ = (head_ + 1) % data_.size();
        ++size_;
    }

    T pop_unchecked() {
        T value = std::move(data_[tail_]);
        tail_ = (tail_ + 1) % data_.size();
        --size_;
        return value;
    }

    template <typename... Args>
    void emplace_unchecked(Args&&... args) {
        data_[head_] = T(std::forward<Args>(args)...);
        head_ = (head_ + 1) % data_.size();
        ++size_;
    }

    std::array<T, Capacity> data_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t size_ = 0;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    bool closed_ = false;
};

} // namespace spsc
