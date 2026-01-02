#include "nanomq/queue.hpp"
#include "nanomq/message.hpp"
#include <benchmark/benchmark.h>
#include <thread>

using namespace nanomq;

// Benchmark: CPU utilization under load
static void BM_CPUUtilization(benchmark::State& state) {
    SPSCQueue<Message, 65536> queue;
    const int num_messages = state.range(0);
    std::atomic<bool> stop{false};
    
    std::thread consumer([&]() {
        int count = 0;
        while (!stop) {
            Message msg;
            if (queue.try_pop(msg)) {
                ++count;
                // Simulate some work
                benchmark::DoNotOptimize(msg);
            } else {
                std::this_thread::yield();
            }
        }
    });
    
    for (auto _ : state) {
        for (int i = 0; i < num_messages; ++i) {
            Message msg;
            msg.header.id = i;
            msg.header.timestamp = get_timestamp_ns();
            msg.header.size = 1024;
            
            while (!queue.try_push(msg)) {
                std::this_thread::yield();
            }
        }
    }
    
    stop = true;
    consumer.join();
    
    state.SetItemsProcessed(state.iterations() * num_messages);
}
BENCHMARK(BM_CPUUtilization)
    ->Arg(1000)->Arg(10000)->Arg(100000)
    ->Threads(2)
    ->UseRealTime();

// Benchmark: Idle behavior (no messages)
static void BM_IdleBehavior(benchmark::State& state) {
    SPSCQueue<int, 65536> queue;
    
    for (auto _ : state) {
        int value;
        bool result = queue.try_pop(value);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_IdleBehavior);

// CPU pause instruction (platform-independent)
inline void cpu_pause() {
#if defined(__x86_64__) || defined(_M_X64)
    __builtin_ia32_pause();  // x86 PAUSE
#elif defined(__aarch64__) || defined(_M_ARM64)
    __asm__ __volatile__("yield");  // ARM YIELD
#else
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

// Benchmark: Contention with spinning
static void BM_ContentionSpinning(benchmark::State& state) {
    SPSCQueue<int, 1024> queue;  // Smaller queue for more contention
    std::atomic<bool> stop{false};
    
    std::thread consumer([&]() {
        while (!stop) {
            int value;
            if (!queue.try_pop(value)) {
                // Spin without yielding
                for (int i = 0; i < 10; ++i) {
                    cpu_pause();
                }
            }
        }
    });
    
    for (auto _ : state) {
        int value = 42;
        while (!queue.try_push(value)) {
            // Spin without yielding
            for (int i = 0; i < 10; ++i) {
                cpu_pause();
            }
        }
    }
    
    stop = true;
    consumer.join();
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ContentionSpinning)->Threads(2);

BENCHMARK_MAIN();

