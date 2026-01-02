# NanoMQ Architecture

This document describes the internal architecture and design decisions of NanoMQ.

## Design Goals

1. **Ultra-Low Latency**: P99 < 10μs for in-memory operations
2. **High Throughput**: 1M+ messages/sec per topic
3. **Reliability**: No message loss with crash recovery
4. **Simplicity**: Clean API, easy to understand and maintain
5. **Zero-Copy**: Minimize data copies in critical paths

## Core Components

### 1. Lock-Free SPSC Ring Buffer

**File**: `include/nanomq/queue.hpp`

The foundation of NanoMQ is a Single Producer Single Consumer (SPSC) lock-free ring buffer implemented using atomic operations.

**Key Features**:
- **Capacity**: Power-of-2 for fast modulo via bitwise AND
- **Atomics**: Head (producer) and tail (consumer) indices
- **Memory Ordering**: Acquire/release semantics for synchronization
- **Cache-Line Alignment**: Prevents false sharing (64-byte alignment)

**Design Rationale**:
- SPSC avoids complex CAS loops needed for MPSC
- Acquire/release is faster than sequential consistency
- Cache-line alignment eliminates false sharing penalties

```
Memory Layout:
┌─────────────────────────────────────┐
│  head_ (64 bytes, cache-aligned)    │ ← Producer writes
├─────────────────────────────────────┤
│  tail_ (64 bytes, cache-aligned)    │ ← Consumer writes
├─────────────────────────────────────┤
│  storage_ (pointer to ring buffer)  │
└─────────────────────────────────────┘

Ring Buffer (separate allocation):
[item 0][item 1][item 2]...[item N-1]
   ^                            ^
  tail                        head
```

**Critical Path**:
1. Producer loads head (relaxed)
2. Producer loads tail (acquire) - sees all consumer writes
3. Check if full: `(head + 1) & MASK == tail`
4. Write item at head position
5. Update head (release) - makes write visible to consumer

### 2. Message Structure

**File**: `include/nanomq/message.hpp`

Messages consist of a fixed-size header and variable-size payload.

**Header Layout** (64 bytes, cache-line aligned):
```
Bytes   Field           Description
0-7     id              Unique message ID (monotonic)
8-15    timestamp       Unix timestamp (nanoseconds)
16-19   topic_id        Numeric topic identifier
20-23   size            Payload size (max 64KB)
24-27   crc32           CRC32 checksum for integrity
28-31   flags           Bit flags (compressed, persistent, etc.)
32-63   padding         Reserved (aligns to 64 bytes)
```

**Zero-Copy Design**:
- Payload pointer (`data`) points to external memory
- No payload copying when passing messages through queue
- Payload ownership managed by caller

**Checksum**:
- CRC32 for corruption detection
- Calculated on write, verified on read
- Hardware-accelerated on modern CPUs

### 3. Persistence Layer

**Files**: `src/storage/mmap_file.cpp`, `src/storage/wal.cpp`

#### Write-Ahead Log (WAL)

Messages are appended to a write-ahead log for durability.

**Design**:
- Memory-mapped files for zero-copy writes
- Sequential appends (no seeks)
- Periodic fsync (default: 10ms)
- Segment rotation at 100MB

**Recovery**:
1. Scan WAL segments on startup
2. Replay messages to rebuild in-memory state
3. Verify checksums to detect corruption
4. Resume from last committed position

**Format**:
```
[Header][Message 1][Header][Message 2]...

Header (16 bytes):
- Magic: 0x4E4D5121 ('NMQ!')
- Length: Message size
- CRC32: Checksum of message
- Type: Message type (data, commit, etc.)
```

#### Memory-Mapped I/O

**Advantages**:
- Zero-copy: Kernel maps file pages directly to process
- Lazy loading: Pages loaded on access
- OS page cache: Automatic caching by kernel

**Trade-offs**:
- Page faults on first access (mitigated by madvise)
- Memory pressure: OS can evict pages under load
- Write guarantees: Need msync for durability

### 4. Network Layer

**Files**: `src/network/tcp_server.cpp`, `src/network/protocol.cpp`

#### TCP Server

- **Event Loop**: epoll (Linux) or kqueue (macOS)
- **Non-blocking I/O**: Asynchronous socket operations
- **Connection Pool**: Reuse connections to minimize overhead

#### Binary Protocol

**Message Frame**:
```
[4 bytes: type][4 bytes: length][N bytes: payload]

Types:
1 = PUBLISH
2 = SUBSCRIBE
3 = UNSUBSCRIBE
4 = ACK
5 = DATA
```

**Optimizations**:
- Nagle-like batching: Flush every 10ms or 1KB
- Zero-copy: sendfile() for large payloads
- Optional compression: LZ4 for messages > threshold

### 5. Broker Architecture

**Files**: `src/broker/broker.cpp`, `src/broker/topic.cpp`

#### Topic Management

Each topic has:
- Ring buffer for message queue
- Subscription list (consumers)
- Message ID counter (monotonic)
- Retention policy (time/size-based)

```
Topic Structure:
┌─────────────────────┐
│  Topic: "orders"    │
├─────────────────────┤
│  Ring Buffer (64K)  │ ← Message queue
├─────────────────────┤
│  Subscriptions []   │ ← Active consumers
├─────────────────────┤
│  Message ID: 12345  │ ← Next ID
└─────────────────────┘
```

#### Message Flow

**Publish**:
1. Client sends PUBLISH to broker
2. Broker routes to topic's ring buffer
3. Append to WAL (async)
4. Notify subscribers

**Subscribe**:
1. Client sends SUBSCRIBE to broker
2. Broker adds to topic's subscription list
3. Client polls for messages
4. Broker pops from ring buffer and sends

**Consumer Groups**:
- Multiple consumers share a topic
- Messages distributed round-robin
- Each consumer has independent position

### 6. Client API

**Files**: `src/api/publisher_impl.cpp`, `src/api/subscriber_impl.cpp`

#### Publisher

**Buffering**:
- Internal buffer for batching
- Flush on timeout (10ms) or size (1KB)
- Explicit flush() for immediate send

**Compression**:
- Optional LZ4 compression
- Threshold-based (default: 4KB)
- Trades CPU for network bandwidth

#### Subscriber

**Polling**:
- Blocking poll with timeout
- Batch poll for multiple messages
- Zero-copy: Messages reference shared memory

**Acknowledgment**:
- Explicit commit required (at-least-once)
- Batch commit for efficiency
- Position tracked per consumer

## Performance Optimizations

### 1. Cache-Line Alignment

**Problem**: False sharing when multiple threads access nearby memory
**Solution**: Align atomics to 64-byte cache lines

```cpp
alignas(64) std::atomic<size_t> head_;  // Separate cache line
alignas(64) std::atomic<size_t> tail_;  // Separate cache line
```

### 2. Memory Ordering

**Problem**: Sequential consistency is expensive (full barriers)
**Solution**: Use acquire/release for SPSC synchronization

- **Release** (producer): Ensures writes visible to consumer
- **Acquire** (consumer): Sees all producer writes
- **Relaxed** (same thread): No synchronization needed

### 3. Batch Operations

**Problem**: Per-message overhead dominates at high throughput
**Solution**: Batch up to 256 messages in one operation

**Speedup**: 2.8× vs single-message operations

### 4. Zero-Copy

**Problem**: Copying large payloads wastes CPU and memory bandwidth
**Solution**: Pass pointers instead of copying data

- Ring buffer stores pointers, not payloads
- mmap for file I/O (kernel zero-copy)
- sendfile for network I/O (kernel zero-copy)

## Scalability

### Horizontal Scaling

- Multiple topics (independent ring buffers)
- Multiple brokers (shard topics across brokers)
- Replication (future work)

### Vertical Scaling

- CPU pinning (producer/consumer on separate cores)
- NUMA awareness (allocate on local node)
- Huge pages (reduce TLB misses)

## Trade-offs

### SPSC vs MPSC

**SPSC Pros**:
- Simpler implementation
- Lower latency (no CAS loops)
- Better cache behavior

**SPSC Cons**:
- One producer per topic
- Multiple topics needed for many producers

**Decision**: SPSC is optimal for common case (one writer per topic)

### At-Least-Once vs Exactly-Once

**At-Least-Once**:
- Simpler to implement
- Better performance
- Duplicates possible (idempotent consumers)

**Exactly-Once**:
- Complex (distributed transactions)
- Higher latency
- Stronger guarantees

**Decision**: At-least-once by default, exactly-once as future extension

### Memory vs Disk

**Pure Memory**:
- Fastest (no disk I/O)
- Volatile (data loss on crash)

**Persistent**:
- Durable (survives crashes)
- Slower (disk write latency)

**Decision**: Hybrid - async WAL for durability with minimal latency impact

## Future Improvements

1. **MPSC Support**: Multiple producers per topic (CAS-based)
2. **Consumer Groups**: Load balancing across consumers
3. **Exactly-Once**: Distributed transactions for strong guarantees
4. **Replication**: Multi-broker consensus (Raft)
5. **Kernel Bypass**: io_uring or DPDK for ultra-low latency
6. **Compression**: LZ4/Zstd for large messages
7. **TLS**: Encrypted connections
8. **Admin API**: HTTP REST for monitoring and management

## References

- [LMAX Disruptor](https://github.com/LMAX-Exchange/disruptor)
- [Aeron](https://github.com/real-logic/aeron)
- [Mechanical Sympathy](https://mechanical-sympathy.blogspot.com/)
- [C++ Memory Model](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [Linux mmap()](https://man7.org/linux/man-pages/man2/mmap.2.html)

---

**Questions?** Open a GitHub Discussion or contact the maintainers.

