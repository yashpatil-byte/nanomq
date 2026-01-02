#include "nanomq/nanomq.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

int main() {
    std::cout << "NanoMQ Simple Pub/Sub Example\n";
    std::cout << "==============================\n\n";

    const std::string broker_addr = "127.0.0.1:9000";
    const std::string topic = "test-topic";

    // Create subscriber in a separate thread
    std::thread subscriber_thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        nanomq::Subscriber sub(broker_addr);
        if (!sub.subscribe(topic)) {
            std::cerr << "Failed to subscribe to " << topic << "\n";
            return;
        }

        std::cout << "[Subscriber] Subscribed to '" << topic << "'\n";

        // Poll for 5 messages
        for (int i = 0; i < 5; ++i) {
            nanomq::Message msg = sub.poll(5000000);  // 5 second timeout
            if (msg.header.id > 0) {
                std::string payload(reinterpret_cast<char*>(msg.data),
                                   msg.header.size);
                std::cout << "[Subscriber] Received: " << payload
                         << " (ID: " << msg.header.id << ")\n";
                sub.commit(msg.header.id);
            }
        }

        std::cout << "[Subscriber] Finished\n";
    });

    // Publisher in main thread
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    nanomq::Publisher pub(broker_addr);
    std::cout << "[Publisher] Connected to broker\n";

    // Publish 5 messages
    for (int i = 0; i < 5; ++i) {
        std::string message = "Hello from NanoMQ #" + std::to_string(i);
        uint64_t msg_id = pub.publish(topic, message.c_str(), message.size());
        
        if (msg_id > 0) {
            std::cout << "[Publisher] Published: " << message
                     << " (ID: " << msg_id << ")\n";
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    pub.flush();
    std::cout << "[Publisher] Finished\n";

    subscriber_thread.join();

    std::cout << "\nExample completed!\n";
    return 0;
}

