#include "nanomq/message.hpp"
#include "nanomq/queue.hpp"
#include <string>
#include <vector>

namespace nanomq {

// Topic management
class Topic {
public:
    explicit Topic(const std::string& name)
        : name_(name), message_id_counter_(0) {}

    const std::string& name() const { return name_; }

    // Add a message to the topic
    bool add_message(const Message& msg) {
        // TODO: Add to ring buffer
        (void)msg;
        return true;
    }

    // Get next message ID
    uint64_t next_message_id() {
        return ++message_id_counter_;
    }

private:
    std::string name_;
    uint64_t message_id_counter_;
    SPSCQueue<Message, 65536> queue_;
};

}  // namespace nanomq

