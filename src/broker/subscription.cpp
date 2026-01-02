#include <string>
#include <vector>

namespace nanomq {

// Subscription tracking
class Subscription {
public:
    Subscription(const std::string& topic, const std::string& consumer_group)
        : topic_(topic), consumer_group_(consumer_group), position_(0) {}

    const std::string& topic() const { return topic_; }
    const std::string& consumer_group() const { return consumer_group_; }
    
    uint64_t position() const { return position_; }
    void set_position(uint64_t pos) { position_ = pos; }

private:
    std::string topic_;
    std::string consumer_group_;
    uint64_t position_;  // Last committed message ID
};

}  // namespace nanomq

