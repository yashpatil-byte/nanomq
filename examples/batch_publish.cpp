#include "nanomq/nanomq.hpp"
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    std::cout << "NanoMQ Batch Publishing Example\n";
    std::cout << "================================\n\n";

    const std::string broker_addr = "127.0.0.1:9000";
    const std::string topic = "batch-topic";

    nanomq::Publisher pub(broker_addr);
    std::cout << "Connected to broker\n";

    // Prepare batch data
    const size_t BATCH_SIZE = 100;
    std::vector<std::string> messages;
    std::vector<const void*> data_ptrs;
    std::vector<size_t> sizes;

    for (size_t i = 0; i < BATCH_SIZE; ++i) {
        messages.push_back("Batch message #" + std::to_string(i));
    }

    for (const auto& msg : messages) {
        data_ptrs.push_back(msg.c_str());
        sizes.push_back(msg.size());
    }

    // Measure batch publish time
    auto start = std::chrono::high_resolution_clock::now();
    
    size_t published = pub.publish_batch(topic, data_ptrs.data(),
                                         sizes.data(), BATCH_SIZE);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();

    std::cout << "Published " << published << " messages in " << duration
              << " μs\n";
    std::cout << "Average: " << (duration / published) << " μs/message\n";
    std::cout << "Throughput: " << (published * 1000000 / duration)
              << " msg/sec\n";

    pub.flush();

    std::cout << "\nBatch example completed!\n";
    return 0;
}

