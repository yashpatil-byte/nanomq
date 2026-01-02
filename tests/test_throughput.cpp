#include "nanomq/queue.hpp"
#include "nanomq/message.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace nanomq;

// Test throughput with different message sizes
TEST(ThroughputTest, SingleProducerConsumer) {
    SPSCQueue<Message, 65536> queue;
    const int NUM_MESSAGES = 1000000;
    const int MESSAGE_SIZE = 1024;  // 1KB messages
    
    std::atomic<bool> done{false};
    uint64_t messages_consumed = 0;

    auto start = std::chrono::high_resolution_clock::now();

    // Producer
    std::thread producer([&]() {
        for (int i = 0; i < NUM_MESSAGES; ++i) {
            Message msg;
            msg.header.id = i;
            msg.header.timestamp = get_timestamp_ns();
            msg.header.size = MESSAGE_SIZE;
            
            while (!queue.try_push(msg)) {
                std::this_thread::yield();
            }
        }
    });

    // Consumer
    std::thread consumer([&]() {
        while (messages_consumed < NUM_MESSAGES) {
            Message msg;
            if (queue.try_pop(msg)) {
                ++messages_consumed;
            } else {
                std::this_thread::yield();
            }
        }
        done = true;
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();

    double throughput_msg_sec = (NUM_MESSAGES * 1000.0) / duration;
    double throughput_mb_sec = (NUM_MESSAGES * MESSAGE_SIZE * 1000.0) / (duration * 1024 * 1024);

    std::cout << "Throughput test results:\n";
    std::cout << "  Messages: " << NUM_MESSAGES << "\n";
    std::cout << "  Duration: " << duration << " ms\n";
    std::cout << "  Throughput: " << static_cast<uint64_t>(throughput_msg_sec)
              << " msg/sec\n";
    std::cout << "  Throughput: " << throughput_mb_sec << " MB/sec\n";

    // Target: > 1M messages/sec
    EXPECT_GT(throughput_msg_sec, 1000000);
}

// Test batch throughput
TEST(ThroughputTest, BatchOperations) {
    SPSCQueue<int, 65536> queue;
    const int NUM_BATCHES = 10000;
    const int BATCH_SIZE = 256;
    const int TOTAL_ITEMS = NUM_BATCHES * BATCH_SIZE;
    
    std::vector<int> batch(BATCH_SIZE);
    for (int i = 0; i < BATCH_SIZE; ++i) {
        batch[i] = i;
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer([&]() {
        for (int i = 0; i < NUM_BATCHES; ++i) {
            while (queue.try_push_batch(batch.data(), BATCH_SIZE) != BATCH_SIZE) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        std::vector<int> out(BATCH_SIZE);
        for (int i = 0; i < NUM_BATCHES; ++i) {
            while (queue.try_pop_batch(out.data(), BATCH_SIZE) != BATCH_SIZE) {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();

    double throughput = (TOTAL_ITEMS * 1000.0) / duration;

    std::cout << "Batch throughput test:\n";
    std::cout << "  Total items: " << TOTAL_ITEMS << "\n";
    std::cout << "  Batch size: " << BATCH_SIZE << "\n";
    std::cout << "  Duration: " << duration << " ms\n";
    std::cout << "  Throughput: " << static_cast<uint64_t>(throughput)
              << " items/sec\n";
}

// Memory overhead test
TEST(ThroughputTest, MemoryOverhead) {
    // Test memory usage of different queue sizes
    std::cout << "Memory overhead:\n";
    std::cout << "  SPSCQueue<int, 1024>: " << sizeof(SPSCQueue<int, 1024>) << " bytes\n";
    std::cout << "  SPSCQueue<int, 65536>: " << sizeof(SPSCQueue<int, 65536>) << " bytes\n";
    std::cout << "  SPSCQueue<Message, 1024>: " << sizeof(SPSCQueue<Message, 1024>) << " bytes\n";
    std::cout << "  Message struct: " << sizeof(Message) << " bytes\n";
    std::cout << "  MessageHeader: " << sizeof(MessageHeader) << " bytes\n";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

