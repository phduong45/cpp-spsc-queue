#include "bounded_queue.h"

#include <cassert>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

struct Job {
    int id = 0;
    std::string name;
};

int main() {
    static_assert(spsc::BoundedQueue<int, 4>::capacity() == 4);
    static_assert(spsc::BoundedQueue<std::string, 2>::capacity() == 2);

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

    spsc::BoundedQueue<std::string, 2> move_queue;

    std::string original = "move-me";
    assert(move_queue.try_push(std::move(original)));

    std::string moved_out;
    assert(move_queue.try_pop(moved_out));
    assert(moved_out == "move-me");

    std::cout << "move queue ok\n";

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

    return 0;
}
