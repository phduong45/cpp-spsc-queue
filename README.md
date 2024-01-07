# cpp-spsc-queue

A C++ pet project for building a queue.

## Implementations

### BoundedQueue

A mutex-based bounded blocking queue.

Features:

- fixed capacity
- blocking `push` / `pop`
- non-blocking `try_push` / `try_pop`
- `emplace` / `try_emplace`
- timed `push_for` / `pop_for`
- `close()`
- generic `BoundedQueue<T, Capacity>`

### SpscQueue

A lock-free single-producer single-consumer queue.

Contract:

- exactly one producer thread
- exactly one consumer thread
- capacity must be a power of two
- non-blocking API
- supports move-only types

Implementation notes:

- atomic monotonic `head` / `tail`
- acquire/release memory ordering
- cache-line aligned counters
- raw storage and placement new
- destructor cleans live items

## Build

```bash
make
```

## Test

```bash
make test
```
