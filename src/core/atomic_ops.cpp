#include <atomic>
#include <cstdint>

namespace nanomq {

// Atomic operation primitives
// These provide low-level synchronization primitives

// Compare-and-swap (CAS) operation
// Returns true if successful, false otherwise
bool compare_and_swap_64(std::atomic<uint64_t>* ptr, uint64_t* expected,
                         uint64_t desired) {
    return ptr->compare_exchange_strong(*expected, desired,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire);
}

bool compare_and_swap_32(std::atomic<uint32_t>* ptr, uint32_t* expected,
                         uint32_t desired) {
    return ptr->compare_exchange_strong(*expected, desired,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire);
}

// Atomic fetch-and-add
uint64_t fetch_and_add_64(std::atomic<uint64_t>* ptr, uint64_t value) {
    return ptr->fetch_add(value, std::memory_order_acq_rel);
}

uint32_t fetch_and_add_32(std::atomic<uint32_t>* ptr, uint32_t value) {
    return ptr->fetch_add(value, std::memory_order_acq_rel);
}

// Memory barriers
void memory_barrier_acquire() {
    std::atomic_thread_fence(std::memory_order_acquire);
}

void memory_barrier_release() {
    std::atomic_thread_fence(std::memory_order_release);
}

void memory_barrier_full() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

// Spin wait with exponential backoff
void spin_wait(uint32_t iterations) {
    for (uint32_t i = 0; i < iterations; ++i) {
#if defined(__x86_64__) || defined(_M_X64)
        __builtin_ia32_pause();  // x86 PAUSE instruction
#elif defined(__aarch64__) || defined(_M_ARM64)
        __asm__ __volatile__("yield");  // ARM YIELD instruction
#else
        // Generic busy wait
        std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
    }
}

}  // namespace nanomq

