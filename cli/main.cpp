#include "nanomq/nanomq.hpp"
#include <iostream>
#include <cstring>
#include <chrono>

void print_usage() {
    std::cout << "NanoMQ CLI v1.0.0\n";
    std::cout << "Usage:\n";
    std::cout << "  nanomq-cli publish --topic TOPIC --message MESSAGE [--broker ADDR]\n";
    std::cout << "  nanomq-cli subscribe --topic TOPIC [--broker ADDR]\n";
    std::cout << "  nanomq-cli --help\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --broker ADDR      Broker address (default: 127.0.0.1:9000)\n";
    std::cout << "  --topic TOPIC      Topic name\n";
    std::cout << "  --message MSG      Message to publish\n";
    std::cout << "  --help             Show this help\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const char* command = argv[1];
    const char* broker = "127.0.0.1:9000";
    const char* topic = nullptr;
    const char* message = nullptr;

    // Parse arguments
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--broker") == 0 && i + 1 < argc) {
            broker = argv[++i];
        } else if (strcmp(argv[i], "--topic") == 0 && i + 1 < argc) {
            topic = argv[++i];
        } else if (strcmp(argv[i], "--message") == 0 && i + 1 < argc) {
            message = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }
    }

    if (strcmp(command, "publish") == 0) {
        if (!topic || !message) {
            std::cerr << "Error: --topic and --message required for publish\n";
            return 1;
        }

        auto start = std::chrono::high_resolution_clock::now();
        
        nanomq::Publisher pub(broker);
        uint64_t msg_id = pub.publish(topic, message, strlen(message));
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count();

        if (msg_id > 0) {
            std::cout << "Published message " << msg_id << " to '" << topic
                     << "' (" << duration << "μs)\n";
        } else {
            std::cerr << "Failed to publish message\n";
            return 1;
        }

    } else if (strcmp(command, "subscribe") == 0) {
        if (!topic) {
            std::cerr << "Error: --topic required for subscribe\n";
            return 1;
        }

        nanomq::Subscriber sub(broker);
        if (!sub.subscribe(topic)) {
            std::cerr << "Failed to subscribe to topic '" << topic << "'\n";
            return 1;
        }

        std::cout << "[Waiting for messages on '" << topic << "'...]\n";

        while (true) {
            nanomq::Message msg = sub.poll(1000000);  // 1 second timeout
            if (msg.header.id > 0) {
                std::string payload(reinterpret_cast<char*>(msg.data),
                                   msg.header.size);
                std::cout << "Message " << msg.header.id << ": " << payload
                         << " (timestamp: " << msg.header.timestamp << ")\n";
                sub.commit(msg.header.id);
            }
        }

    } else {
        std::cerr << "Unknown command: " << command << "\n";
        print_usage();
        return 1;
    }

    return 0;
}

