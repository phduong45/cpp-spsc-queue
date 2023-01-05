#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

class OneSlotMailbox {
  public:
    void send(int value) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !has_value_; });

            value_ = value;
            has_value_ = true;
        }

        cv_.notify_one();
    }

    int receive() {
        int value{0};
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return has_value_; });

            value = value_;
            has_value_ = false;
        }

        cv_.notify_one();

        return value;
    }

  private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int value_ = 0;
    bool has_value_ = false;
};

int main() {
    OneSlotMailbox mailbox;
    int first = 0;
    int second = 0;

    std::thread producer([&] {
        mailbox.send(10);
        mailbox.send(20);
    });

    std::thread consumer([&] {
        first = mailbox.receive();
        second = mailbox.receive();
    });

    producer.join();
    consumer.join();

    assert(first == 10);
    assert(second == 20);
    std::cout << "one slot mailbox with backpressure ok\n";
}