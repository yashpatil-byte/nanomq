#pragma once

#include "nanomq/message.hpp"
#include <string>
#include <vector>
#include <memory>

namespace nanomq {

// Forward declarations
class BrokerClient;

// Subscriber API for receiving messages from topics
class Subscriber {
public:
    // Connect to broker at specified address
    explicit Subscriber(const std::string& broker_address = "127.0.0.1:9000",
                       const std::string& consumer_group = "");
    ~Subscriber();

    // Disable copy, allow move
    Subscriber(const Subscriber&) = delete;
    Subscriber& operator=(const Subscriber&) = delete;
    Subscriber(Subscriber&&) noexcept;
    Subscriber& operator=(Subscriber&&) noexcept;

    // Subscribe to a topic
    bool subscribe(const std::string& topic);

    // Unsubscribe from a topic
    bool unsubscribe(const std::string& topic);

    // Poll for a single message (blocks up to timeout_us microseconds)
    // Returns empty Message (id=0) if timeout or no messages
    Message poll(uint64_t timeout_us = 1000000);  // Default 1 second

    // Poll for a batch of messages (up to max_msgs)
    // Returns vector of messages (may be empty)
    std::vector<Message> poll_batch(size_t max_msgs = 256,
                                     uint64_t timeout_us = 1000000);

    // Commit (acknowledge) a message
    // Required for at-least-once delivery semantics
    void commit(uint64_t message_id);

    // Commit a batch of messages
    void commit_batch(const std::vector<uint64_t>& message_ids);

    // Seek to a specific message ID (replay from this point)
    bool seek(uint64_t message_id);

    // Seek to a specific timestamp (replay from this time)
    bool seek_to_timestamp(uint64_t timestamp_ns);

    // Get connection status
    bool is_connected() const;

    // Get current position (latest committed message ID)
    uint64_t get_position() const;

    // Get statistics
    struct Stats {
        uint64_t messages_received;
        uint64_t bytes_received;
        uint64_t messages_committed;
        uint64_t avg_latency_us;
    };
    Stats get_stats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace nanomq

