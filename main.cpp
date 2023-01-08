#include <array>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <thread>

class BoundedQueue {
  public:
    void push(int value) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_full_.wait(lock, [this] { return size_ < data_.size(); });

            data_[head_] = value;
            head_ = (head_ + 1) % data_.size();
            ++size_;
        }

        not_empty_.notify_one();
    }

    int pop() {
        int value{0};
        {
            std::unique_lock<std::mutex> lock(mutex_);
            not_empty_.wait(lock, [this] { return size_ > 0; });

            value = data_[tail_];
            tail_ = (tail_ + 1) % data_.size();
            --size_;
        }

        not_full_.notify_one();

        return value;
    }

  private:
    std::array<int, 4> data_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t size_ = 0;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
};

int main() {
    BoundedQueue queue;
    int sum = 0;

    std::thread producer([&] {
        for (int value = 1; value <= 10; ++value) {
            queue.push(value);
        }
    });

    std::thread consumer([&] {
        for (int i = 0; i < 10; ++i) {
            sum += queue.pop();
        }
    });

    producer.join();
    consumer.join();

    assert(sum == 55);
    std::cout << "bounded queue ok\n";

    return 0;
}