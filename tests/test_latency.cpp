#include "nanomq/queue.hpp"
#include "nanomq/message.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <algorithm>

using namespace nanomq;

// Latency test helper
struct LatencyStats {
    uint64_t min;
    uint64_t max;
    uint64_t mean;
    uint64_t p50;
    uint64_t p99;
    uint64_t p999;
};

LatencyStats calculate_stats(std::vector<uint64_t>& latencies) {
    std::sort(latencies.begin(), latencies.end());
    
    size_t n = latencies.size();
    uint64_t sum = 0;
    for (uint64_t lat : latencies) {
        sum += lat;
    }
    
    return LatencyStats{
        latencies[0],
        latencies[n - 1],
        sum / n,
        latencies[n / 2],
        latencies[n * 99 / 100],
        latencies[n * 999 / 1000]
    };
}

// Test single-threaded latency
TEST(LatencyTest, SingleThreaded) {
    SPSCQueue<int, 65536> queue;
    const int NUM_ITEMS = 10000;
    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_ITEMS);

    for (int i = 0; i < NUM_ITEMS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        queue.try_push(i);
        int value;
        queue.try_pop(value);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start).count();
        latencies.push_back(latency);
    }

    auto stats = calculate_stats(latencies);
    
    std::cout << "Single-threaded latency:\n";
    std::cout << "  Min:  " << stats.min << " ns\n";
    std::cout << "  Mean: " << stats.mean << " ns\n";
    std::cout << "  P50:  " << stats.p50 << " ns\n";
    std::cout << "  P99:  " << stats.p99 << " ns\n";
    std::cout << "  P999: " << stats.p999 << " ns\n";
    std::cout << "  Max:  " << stats.max << " ns\n";
}

// Test concurrent producer/consumer latency
TEST(LatencyTest, ConcurrentProducerConsumer) {
    SPSCQueue<uint64_t, 65536> queue;
    const int NUM_ITEMS = 100000;
    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_ITEMS);

    std::thread producer([&]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            uint64_t timestamp = std::chrono::high_resolution_clock::now()
                                    .time_since_epoch().count();
            while (!queue.try_push(timestamp)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            uint64_t timestamp;
            while (!queue.try_pop(timestamp)) {
                std::this_thread::yield();
            }
            uint64_t now = std::chrono::high_resolution_clock::now()
                              .time_since_epoch().count();
            latencies.push_back(now - timestamp);
        }
    });

    producer.join();
    consumer.join();

    auto stats = calculate_stats(latencies);
    
    std::cout << "Producer-Consumer latency:\n";
    std::cout << "  Min:  " << stats.min << " ns\n";
    std::cout << "  Mean: " << stats.mean << " ns\n";
    std::cout << "  P50:  " << stats.p50 << " ns\n";
    std::cout << "  P99:  " << stats.p99 << " ns (" << stats.p99 / 1000.0 << " μs)\n";
    std::cout << "  P999: " << stats.p999 << " ns (" << stats.p999 / 1000.0 << " μs)\n";
    std::cout << "  Max:  " << stats.max << " ns\n";

    // Target: P99 < 10μs
    EXPECT_LT(stats.p99, 10000) << "P99 latency exceeds 10μs target";
}

// Test batch operation latency
TEST(LatencyTest, BatchOperations) {
    SPSCQueue<int, 65536> queue;
    const int NUM_BATCHES = 1000;
    const int BATCH_SIZE = 256;
    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_BATCHES);

    std::vector<int> batch_data(BATCH_SIZE);
    for (int i = 0; i < BATCH_SIZE; ++i) {
        batch_data[i] = i;
    }

    for (int i = 0; i < NUM_BATCHES; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        queue.try_push_batch(batch_data.data(), BATCH_SIZE);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start).count();
        latencies.push_back(latency);

        // Pop to make room
        std::vector<int> out(BATCH_SIZE);
        queue.try_pop_batch(out.data(), BATCH_SIZE);
    }

    auto stats = calculate_stats(latencies);
    
    std::cout << "Batch operation latency (256 items):\n";
    std::cout << "  Min:  " << stats.min << " ns (" << stats.min / BATCH_SIZE << " ns/item)\n";
    std::cout << "  Mean: " << stats.mean << " ns (" << stats.mean / BATCH_SIZE << " ns/item)\n";
    std::cout << "  P50:  " << stats.p50 << " ns\n";
    std::cout << "  P99:  " << stats.p99 << " ns\n";
    std::cout << "  Max:  " << stats.max << " ns\n";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

