#include "nanomq/message.hpp"
#include <cstring>

namespace nanomq {

// Binary protocol for network communication
// Protocol format:
// [4 bytes: message type] [4 bytes: payload length] [N bytes: payload]

enum MessageType : uint32_t {
    MSG_TYPE_PUBLISH = 1,
    MSG_TYPE_SUBSCRIBE = 2,
    MSG_TYPE_UNSUBSCRIBE = 3,
    MSG_TYPE_ACK = 4,
    MSG_TYPE_DATA = 5,
};

// Encode a message to wire format
size_t encode_message(const Message& msg, uint8_t* buffer, size_t buffer_size) {
    if (buffer_size < sizeof(MessageHeader) + msg.header.size) {
        return 0;
    }

    // Copy header
    std::memcpy(buffer, &msg.header, sizeof(MessageHeader));
    
    // Copy payload
    std::memcpy(buffer + sizeof(MessageHeader), msg.data, msg.header.size);
    
    return sizeof(MessageHeader) + msg.header.size;
}

// Decode a message from wire format
bool decode_message(const uint8_t* buffer, size_t buffer_size, Message& msg) {
    if (buffer_size < sizeof(MessageHeader)) {
        return false;
    }

    // Copy header
    std::memcpy(&msg.header, buffer, sizeof(MessageHeader));
    
    if (buffer_size < sizeof(MessageHeader) + msg.header.size) {
        return false;
    }

    // Point to payload (zero-copy)
    msg.data = const_cast<uint8_t*>(buffer + sizeof(MessageHeader));
    
    return true;
}

}  // namespace nanomq

