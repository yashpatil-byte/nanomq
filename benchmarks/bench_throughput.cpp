#include "nanomq/queue.hpp"
#include "nanomq/message.hpp"
#include <benchmark/benchmark.h>
#include <thread>

using namespace nanomq;

// Benchmark: Sustained throughput with varying message sizes
static void BM_ThroughputVaryingSize(benchmark::State& state) {
    SPSCQueue<Message, 65536> queue;
    const size_t message_size = state.range(0);
    std::atomic<bool> stop{false};
    std::atomic<uint64_t> consumed{0};
    
    // Consumer thread
    std::thread consumer([&]() {
        while (!stop || !queue.is_empty()) {
            Message msg;
            if (queue.try_pop(msg)) {
                ++consumed;
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    // Producer (measured)
    for (auto _ : state) {
        Message msg;
        msg.header.id = 1;
        msg.header.timestamp = get_timestamp_ns();
        msg.header.size = message_size;
        msg.data = nullptr;
        
        while (!queue.try_push(msg)) {
            std::this_thread::yield();
        }
    }
    
    stop = true;
    consumer.join();
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * message_size);
}
BENCHMARK(BM_ThroughputVaryingSize)
    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096)->Arg(65536)
    ->Threads(2);

// Benchmark: Batch throughput
static void BM_BatchThroughput(benchmark::State& state) {
    SPSCQueue<int, 65536> queue;
    const size_t batch_size = state.range(0);
    
    std::vector<int> batch(batch_size);
    for (size_t i = 0; i < batch_size; ++i) {
        batch[i] = i;
    }
    
    std::atomic<bool> stop{false};
    
    std::thread consumer([&]() {
        std::vector<int> out(batch_size);
        while (!stop || !queue.is_empty()) {
            size_t popped = queue.try_pop_batch(out.data(), batch_size);
            if (popped == 0) {
                std::this_thread::yield();
            }
        }
    });
    
    for (auto _ : state) {
        while (queue.try_push_batch(batch.data(), batch_size) != batch_size) {
            std::this_thread::yield();
        }
    }
    
    stop = true;
    consumer.join();
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_BatchThroughput)
    ->Arg(16)->Arg(64)->Arg(256)
    ->Threads(2);

// Benchmark: Maximum throughput (no contention)
static void BM_MaxThroughput(benchmark::State& state) {
    SPSCQueue<uint64_t, 65536> queue;
    
    // Pre-fill queue
    for (size_t i = 0; i < 1000; ++i) {
        queue.try_push(i);
    }
    
    for (auto _ : state) {
        uint64_t value;
        queue.try_pop(value);
        queue.try_push(value);
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MaxThroughput);

BENCHMARK_MAIN();

