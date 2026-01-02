#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace nanomq {

// Lock-free SPSC (Single Producer Single Consumer) ring buffer
// Uses atomic operations with acquire/release semantics for synchronization
template <typename T, size_t Capacity>
class SPSCQueue {
public:
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");

    SPSCQueue() : head_(0), tail_(0) {
        // Pre-allocate storage with cache-line alignment
#ifdef _WIN32
        storage_ = static_cast<T*>(_aligned_malloc(sizeof(T) * Capacity, CACHE_LINE_SIZE));
#else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, CACHE_LINE_SIZE, sizeof(T) * Capacity) != 0) {
            storage_ = nullptr;
        } else {
            storage_ = static_cast<T*>(ptr);
        }
#endif
    }

    ~SPSCQueue() {
#ifdef _WIN32
        _aligned_free(storage_);
#else
        free(storage_);
#endif
    }

    // Disable copy and move
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    // Try to push a single item (producer side)
    // Returns true if successful, false if queue is full
    bool try_push(const T& item) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t next_head = (head + 1) & INDEX_MASK;
        
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false;  // Queue is full
        }

        storage_[head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // Try to pop a single item (consumer side)
    // Returns true if successful, false if queue is empty
    bool try_pop(T& item) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;  // Queue is empty
        }

        item = storage_[tail];
        tail_.store((tail + 1) & INDEX_MASK, std::memory_order_release);
        return true;
    }

    // Batch push (up to max_count items)
    // Returns number of items actually pushed
    size_t try_push_batch(const T* items, size_t max_count) {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_acquire);
        
        size_t available = (tail - head - 1) & INDEX_MASK;
        size_t count = (max_count < available) ? max_count : available;
        
        if (count == 0) {
            return 0;
        }

        size_t new_head = head;
        for (size_t i = 0; i < count; ++i) {
            storage_[new_head] = items[i];
            new_head = (new_head + 1) & INDEX_MASK;
        }

        head_.store(new_head, std::memory_order_release);
        return count;
    }

    // Batch pop (up to max_count items)
    // Returns number of items actually popped
    size_t try_pop_batch(T* items, size_t max_count) {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t head = head_.load(std::memory_order_acquire);
        
        size_t available = (head - tail) & INDEX_MASK;
        size_t count = (max_count < available) ? max_count : available;
        
        if (count == 0) {
            return 0;
        }

        size_t new_tail = tail;
        for (size_t i = 0; i < count; ++i) {
            items[i] = storage_[new_tail];
            new_tail = (new_tail + 1) & INDEX_MASK;
        }

        tail_.store(new_tail, std::memory_order_release);
        return count;
    }

    // Check if queue is empty
    bool is_empty() const {
        return tail_.load(std::memory_order_acquire) == 
               head_.load(std::memory_order_acquire);
    }

    // Check if queue is full
    bool is_full() const {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t next_head = (head + 1) & INDEX_MASK;
        return next_head == tail_.load(std::memory_order_acquire);
    }

    // Get current size (approximate, may be stale)
    size_t size() const {
        const size_t head = head_.load(std::memory_order_acquire);
        const size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail) & INDEX_MASK;
    }

    // Get capacity
    static constexpr size_t capacity() { return Capacity; }

private:
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t INDEX_MASK = Capacity - 1;

    // Cache-line aligned atomics to prevent false sharing
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_;  // Producer writes
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_;  // Consumer writes
    
    T* storage_;  // Ring buffer storage
};

}  // namespace nanomq

