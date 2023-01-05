#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

class OneSlotMailbox {
  public:
    void send(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            value_ = value;
            has_value_ = true;
        }

        cv_.notify_one();
    }

    int receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return has_value_; });

        int value = value_;
        has_value_ = false;
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
    int received = 0;

    std::thread consumer([&] { received = mailbox.receive(); });

    std::thread producer([&] { mailbox.send(42); });

    producer.join();
    consumer.join();

    assert(received == 42);
    std::cout << "one slot mailbox ok\n";
}