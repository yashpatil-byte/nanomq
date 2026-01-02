#include "nanomq/queue.hpp"
#include "nanomq/message.hpp"
#include <benchmark/benchmark.h>
#include <thread>

using namespace nanomq;

// Benchmark: Single-threaded push/pop
static void BM_SingleThreadedPushPop(benchmark::State& state) {
    SPSCQueue<int, 65536> queue;
    
    for (auto _ : state) {
        int value = 42;
        queue.try_push(value);
        int result;
        queue.try_pop(result);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 2);  // push + pop
}
BENCHMARK(BM_SingleThreadedPushPop);

// Benchmark: Message push/pop
static void BM_MessagePushPop(benchmark::State& state) {
    SPSCQueue<Message, 65536> queue;
    
    Message msg;
    msg.header.id = 1;
    msg.header.timestamp = get_timestamp_ns();
    msg.header.size = 1024;
    msg.data = nullptr;
    
    for (auto _ : state) {
        queue.try_push(msg);
        Message result;
        queue.try_pop(result);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MessagePushPop);

// Benchmark: Producer-consumer latency
static void BM_ProducerConsumerLatency(benchmark::State& state) {
    SPSCQueue<uint64_t, 65536> queue;
    std::atomic<bool> stop{false};
    
    std::thread consumer([&]() {
        while (!stop) {
            uint64_t value;
            if (queue.try_pop(value)) {
                benchmark::DoNotOptimize(value);
            } else {
                std::this_thread::yield();
            }
        }
        // Drain remaining items
        uint64_t value;
        while (queue.try_pop(value)) {}
    });
    
    for (auto _ : state) {
        uint64_t timestamp = std::chrono::high_resolution_clock::now()
                                .time_since_epoch().count();
        while (!queue.try_push(timestamp)) {
            std::this_thread::yield();
        }
    }
    
    stop = true;
    consumer.join();
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProducerConsumerLatency)->Threads(2);

// Benchmark: Batch operations
static void BM_BatchPushPop(benchmark::State& state) {
    SPSCQueue<int, 65536> queue;
    const size_t batch_size = state.range(0);
    
    std::vector<int> data(batch_size);
    for (size_t i = 0; i < batch_size; ++i) {
        data[i] = i;
    }
    
    std::vector<int> output(batch_size);
    
    for (auto _ : state) {
        queue.try_push_batch(data.data(), batch_size);
        queue.try_pop_batch(output.data(), batch_size);
        benchmark::DoNotOptimize(output);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_BatchPushPop)->Arg(16)->Arg(64)->Arg(256);

// Benchmark: CRC32 calculation
static void BM_CRC32Calculation(benchmark::State& state) {
    const size_t size = state.range(0);
    std::vector<uint8_t> data(size, 0xAA);
    
    for (auto _ : state) {
        uint32_t crc = Message::calculate_crc32(data.data(), data.size());
        benchmark::DoNotOptimize(crc);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}
BENCHMARK(BM_CRC32Calculation)->Arg(64)->Arg(1024)->Arg(4096)->Arg(65536);

BENCHMARK_MAIN();

