#include "nanomq/queue.hpp"
#include "nanomq/message.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace nanomq;

// Test basic push/pop operations
TEST(SPSCQueueTest, BasicPushPop) {
    SPSCQueue<int, 16> queue;

    EXPECT_TRUE(queue.is_empty());
    EXPECT_FALSE(queue.is_full());

    // Push items
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(queue.try_push(i));
    }

    EXPECT_FALSE(queue.is_empty());
    EXPECT_EQ(queue.size(), 10);

    // Pop items
    for (int i = 0; i < 10; ++i) {
        int value;
        EXPECT_TRUE(queue.try_pop(value));
        EXPECT_EQ(value, i);
    }

    EXPECT_TRUE(queue.is_empty());
}

// Test queue full condition
TEST(SPSCQueueTest, QueueFull) {
    SPSCQueue<int, 16> queue;

    // Fill the queue (capacity - 1 items due to ring buffer design)
    for (int i = 0; i < 15; ++i) {
        EXPECT_TRUE(queue.try_push(i));
    }

    EXPECT_TRUE(queue.is_full());
    EXPECT_FALSE(queue.try_push(999));  // Should fail
}

// Test batch operations
TEST(SPSCQueueTest, BatchOperations) {
    SPSCQueue<int, 256> queue;

    // Batch push
    std::vector<int> items;
    for (int i = 0; i < 100; ++i) {
        items.push_back(i);
    }

    size_t pushed = queue.try_push_batch(items.data(), items.size());
    EXPECT_EQ(pushed, 100);
    EXPECT_EQ(queue.size(), 100);

    // Batch pop
    std::vector<int> popped(50);
    size_t popped_count = queue.try_pop_batch(popped.data(), 50);
    EXPECT_EQ(popped_count, 50);
    
    for (size_t i = 0; i < popped_count; ++i) {
        EXPECT_EQ(popped[i], static_cast<int>(i));
    }

    EXPECT_EQ(queue.size(), 50);
}

// Test concurrent producer/consumer
TEST(SPSCQueueTest, ConcurrentProducerConsumer) {
    SPSCQueue<int, 65536> queue;
    const int NUM_ITEMS = 100000;
    std::atomic<bool> consumer_done{false};

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            while (!queue.try_push(i)) {
                std::this_thread::yield();
            }
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        int expected = 0;
        while (expected < NUM_ITEMS) {
            int value;
            if (queue.try_pop(value)) {
                EXPECT_EQ(value, expected);
                ++expected;
            } else {
                std::this_thread::yield();
            }
        }
        consumer_done = true;
    });

    producer.join();
    consumer.join();

    EXPECT_TRUE(consumer_done);
    EXPECT_TRUE(queue.is_empty());
}

// Test with Message struct
TEST(SPSCQueueTest, MessageQueue) {
    SPSCQueue<Message, 1024> queue;

    // Create test messages
    for (uint64_t i = 0; i < 10; ++i) {
        Message msg;
        msg.header.id = i;
        msg.header.timestamp = get_timestamp_ns();
        msg.header.topic_id = 42;
        msg.header.size = 0;
        msg.data = nullptr;

        EXPECT_TRUE(queue.try_push(msg));
    }

    // Verify messages
    for (uint64_t i = 0; i < 10; ++i) {
        Message msg;
        EXPECT_TRUE(queue.try_pop(msg));
        EXPECT_EQ(msg.header.id, i);
        EXPECT_EQ(msg.header.topic_id, 42);
    }
}

// Performance benchmark (not a unit test, but useful)
TEST(SPSCQueueTest, LatencyBenchmark) {
    SPSCQueue<int, 65536> queue;
    const int NUM_ITEMS = 10000;
    
    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_ITEMS);

    std::thread producer([&]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            while (!queue.try_push(i)) {
                std::this_thread::yield();
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end - start).count();
            latencies.push_back(latency);
        }
    });

    std::thread consumer([&]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            int value;
            while (!queue.try_pop(value)) {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    // Calculate statistics
    std::sort(latencies.begin(), latencies.end());
    uint64_t p50 = latencies[NUM_ITEMS / 2];
    uint64_t p99 = latencies[NUM_ITEMS * 99 / 100];
    uint64_t p999 = latencies[NUM_ITEMS * 999 / 1000];

    std::cout << "Latency P50:  " << p50 << " ns\n";
    std::cout << "Latency P99:  " << p99 << " ns\n";
    std::cout << "Latency P999: " << p999 << " ns\n";

    // Target: P99 < 10μs = 10000 ns
    EXPECT_LT(p99, 10000);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

