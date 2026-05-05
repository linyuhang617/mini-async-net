#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

static std::mutex mtx;

static void bench(int concurrency, int msgs_per_conn) {
    std::vector<long long> latencies;
    std::vector<std::thread> threads;

    for (int i = 0; i < concurrency; i++) {
        threads.emplace_back([&]() {
            int fd = ::socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(9090);
            ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
            if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { ::close(fd); return; }
            char buf[64];
            std::vector<long long> local;
            for (int j = 0; j < msgs_per_conn; j++) {
                auto t0 = std::chrono::steady_clock::now();
                ::write(fd, "ping", 4);
                ::read(fd, buf, sizeof(buf));
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0).count();
                local.push_back(us);
            }
            ::close(fd);
            std::lock_guard<std::mutex> lock(mtx);
            latencies.insert(latencies.end(), local.begin(), local.end());
        });
    }

    auto t0 = std::chrono::steady_clock::now();
    for (auto& t : threads) t.join();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    std::sort(latencies.begin(), latencies.end());
    long long p99 = latencies.empty() ? 0 : latencies[latencies.size() * 99 / 100];
    long long total = concurrency * msgs_per_conn;
    std::cout << "  concurrency=" << concurrency
              << "  msgs=" << total
              << "  time=" << ms << "ms"
              << "  throughput=" << (total * 1000 / (ms ? ms : 1)) << " msg/s"
              << "  p99=" << p99 << "us\n";
}

int main() {
    std::cout << "=== mini-async-net benchmark ===\n";
    bench(100, 20);
    bench(500, 20);
    return 0;
}
