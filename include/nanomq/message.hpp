#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace nanomq {

// Cache line size for alignment (64 bytes on most modern CPUs)
constexpr size_t CACHE_LINE_SIZE = 64;

// Maximum message payload size (64KB)
constexpr size_t MAX_PAYLOAD_SIZE = 65536;

// Message header structure (64 bytes, cache-line aligned)
struct alignas(CACHE_LINE_SIZE) MessageHeader {
    uint64_t id;              // Unique message ID (8 bytes)
    uint64_t timestamp;       // Unix timestamp in nanoseconds (8 bytes)
    uint32_t topic_id;        // Topic identifier (4 bytes)
    uint32_t size;            // Payload size in bytes (4 bytes)
    uint32_t crc32;           // CRC32 checksum of payload (4 bytes)
    uint32_t flags;           // Message flags (4 bytes)
    uint8_t padding[28];      // Pad to 64 bytes

    MessageHeader()
        : id(0), timestamp(0), topic_id(0), size(0), crc32(0), flags(0) {
        std::memset(padding, 0, sizeof(padding));
    }
};

static_assert(sizeof(MessageHeader) == CACHE_LINE_SIZE,
              "MessageHeader must be exactly one cache line");

// Message flags
enum MessageFlags : uint32_t {
    MSG_FLAG_NONE = 0,
    MSG_FLAG_COMPRESSED = 1 << 0,    // Payload is LZ4 compressed
    MSG_FLAG_ENCRYPTED = 1 << 1,     // Payload is encrypted
    MSG_FLAG_PERSISTENT = 1 << 2,    // Must be persisted to disk
    MSG_FLAG_PRIORITY = 1 << 3,      // High-priority message
};

// Message structure with zero-copy design
// The payload pointer points to a memory region (shared memory or mmap)
struct Message {
    MessageHeader header;
    uint8_t* data;  // Pointer to payload (NOT owned by this struct)

    Message() : data(nullptr) {}

    Message(uint64_t id, uint64_t timestamp, uint32_t topic_id,
            const void* payload, size_t payload_size)
        : data(nullptr) {
        header.id = id;
        header.timestamp = timestamp;
        header.topic_id = topic_id;
        header.size = static_cast<uint32_t>(payload_size);
        header.flags = MSG_FLAG_NONE;
        header.crc32 = calculate_crc32(payload, payload_size);
    }

    // Calculate CRC32 checksum
    static uint32_t calculate_crc32(const void* data, size_t size);

    // Verify message integrity
    bool verify_checksum() const {
        return header.crc32 == calculate_crc32(data, header.size);
    }

    // Get timestamp in nanoseconds since epoch
    uint64_t get_timestamp_ns() const { return header.timestamp; }

    // Check if message has a specific flag
    bool has_flag(MessageFlags flag) const {
        return (header.flags & flag) != 0;
    }

    // Set a flag
    void set_flag(MessageFlags flag) { header.flags |= flag; }

    // Clear a flag
    void clear_flag(MessageFlags flag) { header.flags &= ~flag; }
};

// Batch message container for efficient batch operations
struct MessageBatch {
    static constexpr size_t MAX_BATCH_SIZE = 256;

    Message messages[MAX_BATCH_SIZE];
    size_t count;

    MessageBatch() : count(0) {}

    bool is_full() const { return count >= MAX_BATCH_SIZE; }
    bool is_empty() const { return count == 0; }

    void add(const Message& msg) {
        if (!is_full()) {
            messages[count++] = msg;
        }
    }

    void clear() { count = 0; }
};

// Get current time in nanoseconds (for timestamps)
inline uint64_t get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL +
           static_cast<uint64_t>(ts.tv_nsec);
}

}  // namespace nanomq

