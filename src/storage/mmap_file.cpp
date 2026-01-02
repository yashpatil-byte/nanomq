#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

namespace nanomq {

// Memory-mapped file for zero-copy persistence
class MMapFile {
public:
    MMapFile(const char* path, size_t size, bool create = true)
        : fd_(-1), data_(nullptr), size_(0) {
        int flags = O_RDWR;
        if (create) {
            flags |= O_CREAT;
        }

        fd_ = open(path, flags, 0644);
        if (fd_ < 0) {
            throw std::runtime_error("Failed to open file");
        }

        // Resize file if needed
        if (create) {
            if (ftruncate(fd_, size) != 0) {
                close(fd_);
                throw std::runtime_error("Failed to resize file");
            }
        } else {
            struct stat st;
            if (fstat(fd_, &st) != 0) {
                close(fd_);
                throw std::runtime_error("Failed to stat file");
            }
            size = st.st_size;
        }

        // Memory map the file
        data_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (data_ == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("Failed to mmap file");
        }

        size_ = size;
    }

    ~MMapFile() {
        if (data_ != nullptr && data_ != MAP_FAILED) {
            munmap(data_, size_);
        }
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    void* data() { return data_; }
    const void* data() const { return data_; }
    size_t size() const { return size_; }

    // Sync to disk
    void sync() {
        if (data_ != nullptr) {
            msync(data_, size_, MS_SYNC);
        }
    }

    // Async sync
    void async_sync() {
        if (data_ != nullptr) {
            msync(data_, size_, MS_ASYNC);
        }
    }

private:
    int fd_;
    void* data_;
    size_t size_;
};

}  // namespace nanomq

