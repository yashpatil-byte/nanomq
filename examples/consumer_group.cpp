#include "nanomq/nanomq.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>

void consumer_worker(int id, const std::string& broker_addr,
                     const std::string& topic, const std::string& group) {
    nanomq::Subscriber sub(broker_addr, group);
    
    if (!sub.subscribe(topic)) {
        std::cerr << "[Consumer " << id << "] Failed to subscribe\n";
        return;
    }

    std::cout << "[Consumer " << id << "] Started in group '" << group << "'\n";

    // Process messages for 5 seconds
    auto start = std::chrono::steady_clock::now();
    int messages_processed = 0;

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
        nanomq::Message msg = sub.poll(100000);  // 100ms timeout
        
        if (msg.header.id > 0) {
            std::string payload(reinterpret_cast<char*>(msg.data),
                               msg.header.size);
            std::cout << "[Consumer " << id << "] Processed: " << payload
                     << " (ID: " << msg.header.id << ")\n";
            sub.commit(msg.header.id);
            ++messages_processed;
        }
    }

    std::cout << "[Consumer " << id << "] Processed " << messages_processed
              << " messages\n";
}

int main() {
    std::cout << "NanoMQ Consumer Group Example\n";
    std::cout << "=============================\n\n";

    const std::string broker_addr = "127.0.0.1:9000";
    const std::string topic = "shared-topic";
    const std::string consumer_group = "worker-group";

    // Start 3 consumers in the same group
    std::vector<std::thread> consumers;
    for (int i = 1; i <= 3; ++i) {
        consumers.emplace_back(consumer_worker, i, broker_addr, topic,
                              consumer_group);
    }

    // Give consumers time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Publisher sends messages
    nanomq::Publisher pub(broker_addr);
    std::cout << "[Publisher] Sending 20 messages...\n";

    for (int i = 0; i < 20; ++i) {
        std::string message = "Work item #" + std::to_string(i);
        pub.publish(topic, message.c_str(), message.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    pub.flush();
    std::cout << "[Publisher] All messages sent\n";

    // Wait for consumers to finish
    for (auto& thread : consumers) {
        thread.join();
    }

    std::cout << "\nConsumer group example completed!\n";
    std::cout << "Note: Messages should be distributed across consumers\n";
    
    return 0;
}

