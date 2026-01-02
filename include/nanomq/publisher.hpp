#pragma once

#include "nanomq/message.hpp"
#include <string>
#include <memory>

namespace nanomq {

// Forward declarations
class BrokerClient;

// Publisher API for sending messages to topics
class Publisher {
public:
    // Connect to broker at specified address
    explicit Publisher(const std::string& broker_address = "127.0.0.1:9000");
    ~Publisher();

    // Disable copy, allow move
    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;
    Publisher(Publisher&&) noexcept;
    Publisher& operator=(Publisher&&) noexcept;

    // Publish a single message to a topic
    // Returns message ID on success, 0 on failure
    uint64_t publish(const std::string& topic, const void* data, size_t size);

    // Publish a batch of messages (more efficient)
    // Returns number of messages successfully published
    size_t publish_batch(const std::string& topic, const void** data_array,
                         const size_t* size_array, size_t count);

    // Publish with custom message ID and timestamp
    uint64_t publish_message(const std::string& topic, const Message& msg);

    // Force immediate send of buffered messages
    void flush();

    // Set compression for large payloads (threshold in bytes)
    void set_compression_threshold(size_t threshold);

    // Enable/disable batching (default: enabled)
    void set_batching_enabled(bool enabled);

    // Set batch flush interval in microseconds (default: 10ms)
    void set_flush_interval_us(uint64_t interval_us);

    // Get connection status
    bool is_connected() const;

    // Get statistics
    struct Stats {
        uint64_t messages_sent;
        uint64_t bytes_sent;
        uint64_t messages_failed;
        uint64_t avg_latency_us;
    };
    Stats get_stats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace nanomq

