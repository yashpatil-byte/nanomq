#include "nanomq/message.hpp"
#include <gtest/gtest.h>
#include <fstream>

using namespace nanomq;

// Test CRC32 calculation
TEST(PersistenceTest, CRC32Calculation) {
    const char* data = "Hello, NanoMQ!";
    uint32_t crc = Message::calculate_crc32(data, strlen(data));
    
    EXPECT_NE(crc, 0);
    
    // Same data should produce same CRC
    uint32_t crc2 = Message::calculate_crc32(data, strlen(data));
    EXPECT_EQ(crc, crc2);
    
    // Different data should produce different CRC
    const char* data2 = "Hello, World!";
    uint32_t crc3 = Message::calculate_crc32(data2, strlen(data2));
    EXPECT_NE(crc, crc3);
}

// Test message checksum verification
TEST(PersistenceTest, MessageChecksumVerification) {
    const char* payload = "Test message payload";
    Message msg(1, get_timestamp_ns(), 42, payload, strlen(payload));
    
    // Allocate and copy payload
    msg.data = new uint8_t[strlen(payload)];
    memcpy(msg.data, payload, strlen(payload));
    
    // Verify checksum matches
    EXPECT_TRUE(msg.verify_checksum());
    
    // Corrupt the data
    msg.data[0] = ~msg.data[0];
    EXPECT_FALSE(msg.verify_checksum());
    
    delete[] msg.data;
}

// Test message flags
TEST(PersistenceTest, MessageFlags) {
    Message msg;
    
    EXPECT_FALSE(msg.has_flag(MSG_FLAG_COMPRESSED));
    EXPECT_FALSE(msg.has_flag(MSG_FLAG_PERSISTENT));
    
    msg.set_flag(MSG_FLAG_COMPRESSED);
    EXPECT_TRUE(msg.has_flag(MSG_FLAG_COMPRESSED));
    EXPECT_FALSE(msg.has_flag(MSG_FLAG_PERSISTENT));
    
    msg.set_flag(MSG_FLAG_PERSISTENT);
    EXPECT_TRUE(msg.has_flag(MSG_FLAG_COMPRESSED));
    EXPECT_TRUE(msg.has_flag(MSG_FLAG_PERSISTENT));
    
    msg.clear_flag(MSG_FLAG_COMPRESSED);
    EXPECT_FALSE(msg.has_flag(MSG_FLAG_COMPRESSED));
    EXPECT_TRUE(msg.has_flag(MSG_FLAG_PERSISTENT));
}

// Test message batch operations
TEST(PersistenceTest, MessageBatch) {
    MessageBatch batch;
    
    EXPECT_TRUE(batch.is_empty());
    EXPECT_FALSE(batch.is_full());
    
    // Add messages
    for (size_t i = 0; i < 10; ++i) {
        Message msg;
        msg.header.id = i;
        batch.add(msg);
    }
    
    EXPECT_EQ(batch.count, 10);
    EXPECT_FALSE(batch.is_empty());
    EXPECT_FALSE(batch.is_full());
    
    batch.clear();
    EXPECT_TRUE(batch.is_empty());
}

// Test MessageHeader alignment
TEST(PersistenceTest, MessageHeaderAlignment) {
    EXPECT_EQ(sizeof(MessageHeader), CACHE_LINE_SIZE);
    EXPECT_EQ(alignof(MessageHeader), CACHE_LINE_SIZE);
}

// Placeholder for WAL tests (would require file I/O)
TEST(PersistenceTest, WALBasicOperation) {
    // TODO: Test WAL append, flush, recovery
    EXPECT_TRUE(true);  // Placeholder
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

