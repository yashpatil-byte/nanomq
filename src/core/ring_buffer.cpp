#include "nanomq/queue.hpp"
#include <cstdlib>

// Implementation details for ring buffer
// Template implementations are in the header file (queue.hpp)
// This file contains any non-template helper functions

namespace nanomq {

// Helper function to allocate aligned memory
void* aligned_alloc_wrapper(size_t alignment, size_t size) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#endif
}

// Helper function to free aligned memory
void aligned_free_wrapper(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

}  // namespace nanomq

