# mini-async-net

Asynchronous TCP server library driven by epoll, written in C++14.

## Components

- **EventLoop**: wraps epoll, dispatches fd events via callbacks
- **TcpServer**: manages listen socket, accept loop, fd lifecycle, factory pattern
- **Connection**: abstract base class for protocol logic, subclass to customize
- **ThreadPool**: fixed-size pool, submit() returns std::future

## Build

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build

## Run

    ./build/echo_server    # port 9090
    ./build/chat_server    # port 9091
    ./build/bench

## Benchmark

Environment: Intel Xeon E-2136, Ubuntu 24.04, GCC 13.3, -O2

| Concurrency | Throughput (msg/s) | p99 Latency |
|-------------|-------------------|-------------|
| 100         | 86,956            | 3ms         |
| 500         | 8,960             | 33ms        |

Throughput drops at 500 because the single-threaded event loop becomes the bottleneck. p99 grows linearly with concurrency, expected without load balancing.

## Design Notes

- epoll over select/poll: returns only ready events O(1), not O(n) scan
- Non-blocking I/O: EAGAIN lets the loop move on instead of blocking
- weak_ptr in ChatRoom: avoids keeping dead connections alive
- use-after-free fix: EventLoop copies callback before invoking to avoid iterator invalidation (caught by valgrind)

## Resource Management

valgrind --leak-check=full: definitely lost: 0 bytes
