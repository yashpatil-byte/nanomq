#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace nanomq {

// TCP client
class TCPClient {
public:
    TCPClient() : fd_(-1) {}

    ~TCPClient() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    // Connect to server
    bool connect(const char* host, uint16_t port) {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) {
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host, &addr.sin_addr);

        if (::connect(fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
            return false;
        }

        return true;
    }

    // Send data
    ssize_t send(const void* data, size_t size) {
        return ::send(fd_, data, size, 0);
    }

    // Receive data
    ssize_t recv(void* buffer, size_t size) {
        return ::recv(fd_, buffer, size, 0);
    }

private:
    int fd_;
};

}  // namespace nanomq

