#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <functional>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

static std::mutex mtx;

static void bench(int concurrency, int msgs_per_conn) {
    std::vector<long long> latencies;

    struct Arg {
        int msgs_per_conn;
        std::vector<long long>* latencies;
        std::mutex* mtx;
    };

    auto worker = [](void* ptr) -> void* {
        auto* arg = static_cast<Arg*>(ptr);
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9090);
        ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { ::close(fd); return nullptr; }
        char buf[64];
        std::vector<long long> local;
        for (int j = 0; j < arg->msgs_per_conn; j++) {
            auto t0 = std::chrono::steady_clock::now();
            { [[maybe_unused]] auto r = ::write(fd, "ping", 4); }
            ssize_t received = 0;
            while (received < 4) {
                ssize_t n = ::read(fd, buf + received, 4 - received);
                if (n <= 0) break;
                received += n;
            }
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - t0).count();
            local.push_back(us);
        }
        ::close(fd);
        std::lock_guard<std::mutex> lock(*arg->mtx);
        arg->latencies->insert(arg->latencies->end(), local.begin(), local.end());
        return nullptr;
    };

    Arg arg{msgs_per_conn, &latencies, &mtx};

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 256 * 1024); // 256 KB per thread

    std::vector<pthread_t> tids(concurrency);
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < concurrency; i++)
        pthread_create(&tids[i], &attr, worker, &arg);
    for (auto& tid : tids)
        pthread_join(tid, nullptr);
    pthread_attr_destroy(&attr);

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    std::sort(latencies.begin(), latencies.end());
    long long p99 = latencies.empty() ? 0 : latencies[latencies.size() * 99 / 100];
    long long total = (long long)concurrency * msgs_per_conn;
    std::cout << "  concurrency=" << concurrency
              << "  msgs=" << total
              << "  time=" << ms << "ms"
              << "  throughput=" << (total * 1000 / (ms ? ms : 1)) << " msg/s"
              << "  p99=" << p99 << "us\n";
}

int main() {
    std::cout << "=== mini-async-net benchmark ===\n";
    bench(100,  20);
    bench(1000, 20);
    bench(5000, 20);
    return 0;
}
