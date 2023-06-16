#include "bounded_queue.h"
#include "spsc_queue.h"

#include <cassert>
#include <chrono>
#include <iostream>

#include <string>
#include <thread>
#include <utility>

struct Job {
    int id = 0;
    std::string name;
};

void test_basic_blocking_queue() {
    spsc::BoundedQueue<int, 4> queue;
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
}

void test_try_api() {
    spsc::BoundedQueue<int, 4> try_queue;
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
}

void test_close() {
    spsc::BoundedQueue<int, 4> close_queue;
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
}

void test_string_queue() {
    spsc::BoundedQueue<std::string, 2> names;

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
}

void test_move_queue() {
    spsc::BoundedQueue<std::string, 2> move_queue;

    std::string original = "move-me";
    assert(move_queue.try_push(std::move(original)));

    std::string moved_out;
    assert(move_queue.try_pop(moved_out));
    assert(moved_out == "move-me");

    std::cout << "move queue ok\n";
}

void test_try_emplace() {
    spsc::BoundedQueue<Job, 2> jobs;

    assert(jobs.try_emplace(7, "decode"));
    assert(jobs.try_emplace(8, "render"));
    assert(!jobs.try_emplace(9, "upload"));

    Job job;
    assert(jobs.try_pop(job));
    assert(job.id == 7);
    assert(job.name == "decode");

    assert(jobs.try_pop(job));
    assert(job.id == 8);
    assert(job.name == "render");

    std::cout << "emplace queue ok\n";
}

void test_blocking_emplace() {
    spsc::BoundedQueue<Job, 2> blocking_jobs;

    assert(blocking_jobs.emplace(1, "load"));
    assert(blocking_jobs.emplace(2, "store"));
    blocking_jobs.close();

    Job blocking_job;
    assert(blocking_jobs.pop(blocking_job));
    assert(blocking_job.id == 1);
    assert(blocking_job.name == "load");

    assert(blocking_jobs.pop(blocking_job));
    assert(blocking_job.id == 2);
    assert(blocking_job.name == "store");

    assert(!blocking_jobs.emplace(3, "closed"));

    std::cout << "blocking emplace queue ok\n";
}

void test_optional_pop() {
    spsc::BoundedQueue<std::string, 2> optional_queue;

    assert(!optional_queue.try_pop().has_value());

    assert(optional_queue.try_push("alpha"));
    assert(optional_queue.try_push("beta"));

    auto first = optional_queue.try_pop();
    assert(first.has_value());
    assert(*first == "alpha");

    auto second = optional_queue.pop();
    assert(second.has_value());
    assert(*second == "beta");

    optional_queue.close();

    auto closed = optional_queue.pop();
    assert(!closed.has_value());

    std::cout << "optional pop queue ok\n";
}

void test_timed_queue() {
    using namespace std::chrono_literals;

    spsc::BoundedQueue<int, 1> timed_queue;

    assert(timed_queue.push_for(1, 10ms));
    assert(!timed_queue.push_for(2, 10ms));

    auto timed_value = timed_queue.pop_for(10ms);
    assert(timed_value.has_value());
    assert(*timed_value == 1);

    auto missing_value = timed_queue.pop_for(10ms);
    assert(!missing_value.has_value());

    std::cout << "timed queue ok\n";
}

void test_stress_order() {
    constexpr int kCount = 10000;

    spsc::BoundedQueue<int, 8> queue;
    int sum = 0;
    bool result = true;

    std::thread producer([&] {
        for (int value = 1; value <= kCount; ++value) {
            assert(queue.push(value));
        }
        queue.close();
    });

    std::thread consumer([&] {
        int expected = 1;
        while (auto value = queue.pop()) {
            if (*value != expected) {
                result = false;
            }

            sum += *value;
            ++expected;
        }
        assert(expected == kCount + 1);
    });

    producer.join();
    consumer.join();

    assert(result);
    assert(sum == kCount * (kCount + 1) / 2);

    std::cout << "stress test order ok\n";
}

void test_spsc_queue() {
    spsc::SpscQueue<int, 4> queue;

    assert(!queue.try_pop().has_value());

    assert(queue.try_push(1));
    assert(queue.try_push(2));
    assert(queue.try_push(3));
    assert(queue.try_push(4));
    assert(!queue.try_push(5));

    auto first = queue.try_pop();
    assert(first.has_value());
    assert(*first == 1);

    assert(queue.try_push(5));

    for (int expected = 2; expected <= 5; ++expected) {
        auto value = queue.try_pop();
        assert(value.has_value());
        assert(*value == expected);
    }

    assert(!queue.try_pop().has_value());

    std::cout << "spsc monotonic counters ok\n";
}

void test_spsc_two_thread_smoke() {
    constexpr int kCount = 10000;

    spsc::SpscQueue<int, 64> queue;
    int sum = 0;

    std::thread producer([&] {
        for (int value = 1; value <= kCount;) {
            if (queue.try_push(value)) {
                ++value;
            }
        }
    });

    std::thread consumer([&] {
        int received = 0;
        while (received < kCount) {
            auto value = queue.try_pop();
            if (value.has_value()) {
                sum += *value;
                ++received;
            }
        }
    });

    producer.join();
    consumer.join();

    assert(sum == kCount * (kCount + 1) / 2);

    std::cout << "spsc two-thread smoke ok\n";
}

int main() {
    static_assert(spsc::BoundedQueue<int, 4>::capacity() == 4);
    static_assert(spsc::BoundedQueue<std::string, 2>::capacity() == 2);

    test_basic_blocking_queue();
    test_try_api();
    test_close();
    test_string_queue();
    test_move_queue();
    test_try_emplace();
    test_blocking_emplace();
    test_optional_pop();
    test_timed_queue();
    test_stress_order();

    // spsc
    test_spsc_queue();
    test_spsc_two_thread_smoke();

    return 0;
}
