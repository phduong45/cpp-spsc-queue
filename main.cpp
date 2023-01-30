#include <array>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

template <typename T, std::size_t Capacity>
class BoundedQueue {
  public:
    bool push(T value) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_full_.wait(lock, [this] { return !full() || closed_; });

            if (closed_) {
                return false;
            }

            push_unchecked(value);
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

    bool try_push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || full()) {
            return false;
        }

        push_unchecked(value);
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
        data_[head_] = value;
        head_ = (head_ + 1) % data_.size();
        ++size_;
    }

    T pop_unchecked() {
        T value = data_[tail_];
        tail_ = (tail_ + 1) % data_.size();
        --size_;
        return value;
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

int main() {
    BoundedQueue<int, 4> queue;
    int sum = 0;

    std::thread producer([&] {
        for (int value = 1; value <= 10; ++value) {
            assert(queue.push(value));
        }
    });

    std::thread consumer([&] {
        int value = 0;
        for (int i = 0; i < 10; ++i) {
            assert(queue.pop(value));
            sum += value;
        }
    });

    producer.join();
    consumer.join();

    assert(sum == 55);
    std::cout << "bounded queue ok\n";

    BoundedQueue<int, 4> try_queue;
    int value = 0;
    assert(!try_queue.try_pop(value));

    assert(try_queue.try_push(1));
    assert(try_queue.try_push(2));
    assert(try_queue.try_push(3));
    assert(try_queue.try_push(4));
    assert(!try_queue.try_push(5));

    assert(try_queue.try_pop(value));
    assert(value == 1);

    assert(try_queue.try_push(5));
    std::cout << "try queue api ok\n";

    BoundedQueue<int, 4> close_queue;
    assert(close_queue.push(1));
    assert(close_queue.push(2));
    close_queue.close();

    int closed_value = 0;
    assert(close_queue.pop(closed_value));
    assert(closed_value == 1);

    assert(close_queue.pop(closed_value));
    assert(closed_value == 2);

    assert(!close_queue.pop(closed_value));
    assert(!close_queue.push(3));
    assert(!close_queue.try_push(3));

    std::cout << "close queue ok\n";

    BoundedQueue<std::string, 2> names;

    assert(names.try_push("A"));
    assert(names.try_push("B"));
    assert(!names.try_push("C"));

    std::string name;
    assert(names.try_pop(name));
    assert(name == "A");

    assert(names.try_pop(name));
    assert(name == "B");

    assert(!names.try_pop(name));

    std::cout << "string queue ok\n";

    return 0;
}
