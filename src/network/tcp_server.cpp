#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace nanomq {

// TCP server with epoll/kqueue for async I/O
class TCPServer {
public:
    explicit TCPServer(uint16_t port) : listen_fd_(-1), port_(port) {
        // Implementation stub
    }

    ~TCPServer() {
        if (listen_fd_ >= 0) {
            close(listen_fd_);
        }
    }

    // Start listening
    bool start() {
        listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd_ < 0) {
            return false;
        }

        int opt = 1;
        setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);

        if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
            return false;
        }

        if (listen(listen_fd_, 128) < 0) {
            return false;
        }

        return true;
    }

    // Accept a new connection
    int accept_connection() {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        return accept(listen_fd_, (sockaddr*)&client_addr, &len);
    }

private:
    int listen_fd_;
    uint16_t port_;
};

}  // namespace nanomq

