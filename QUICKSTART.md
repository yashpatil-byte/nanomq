# NanoMQ Quick Start Guide

Get up and running with NanoMQ in 5 minutes!

## Prerequisites

```bash
# macOS
brew install cmake

# Ubuntu/Debian
sudo apt-get install cmake build-essential

# Check versions
cmake --version  # Should be 3.15+
g++ --version    # Should be 7+ (C++17 support)
```

## Installation

### Option 1: Build from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/nanomq.git
cd nanomq

# Build (this will download Google Test & Benchmark)
make build

# Run tests to verify installation
make test
```

### Option 2: Docker

```bash
# Build image
make docker-build

# Run broker
make docker-run

# Or manually
docker run -p 9000:9000 nanomq:latest
```

## Your First Message

### Step 1: Start the Broker

In terminal 1:
```bash
cd nanomq
./build/nanomq-broker --port 9000 --data-dir ./data
```

You should see:
```
[INFO] NanoMQ v1.0.0 starting on port 9000
[INFO] Persistence enabled: ./data/wal
[INFO] Topics: 0, Subscribers: 0
```

### Step 2: Subscribe to a Topic

In terminal 2:
```bash
cd nanomq
./build/nanomq-cli subscribe --topic hello-world
```

You should see:
```
[Waiting for messages on 'hello-world'...]
```

### Step 3: Publish a Message

In terminal 3:
```bash
cd nanomq
./build/nanomq-cli publish --topic hello-world --message "Hello, NanoMQ!"
```

You should see in terminal 2:
```
Message 1: Hello, NanoMQ! (timestamp: 1736457123456789)
```

**Congratulations! You just sent your first NanoMQ message! 🎉**

## C++ API Example

Create `my_app.cpp`:

```cpp
#include "nanomq/nanomq.hpp"
#include <iostream>
#include <thread>

int main() {
    // Start subscriber in background
    std::thread subscriber([]() {
        nanomq::Subscriber sub("127.0.0.1:9000");
        sub.subscribe("my-topic");
        
        while (true) {
            nanomq::Message msg = sub.poll(1000000);  // 1 sec timeout
            if (msg.header.id > 0) {
                std::string text((char*)msg.data, msg.header.size);
                std::cout << "Received: " << text << "\n";
                sub.commit(msg.header.id);
            }
        }
    });
    
    // Give subscriber time to connect
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Publish messages
    nanomq::Publisher pub("127.0.0.1:9000");
    for (int i = 0; i < 5; ++i) {
        std::string msg = "Message #" + std::to_string(i);
        pub.publish("my-topic", msg.c_str(), msg.size());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    subscriber.join();
    return 0;
}
```

Compile and run:
```bash
g++ -std=c++17 -I nanomq/include -L nanomq/build -lnanomq my_app.cpp -o my_app
./my_app
```

## Common Tasks

### List All Topics (via Admin API)
```bash
# Coming soon!
curl http://localhost:9000/admin/topics
```

### Monitor Performance
```bash
# Run benchmarks
make benchmark

# Profile with perf (Linux)
make profile
```

### Debug Issues
```bash
# Build with debug symbols and sanitizers
make debug

# Run specific test
./build/test_ring_buffer --gtest_filter=*Concurrent*

# Check for memory leaks
make valgrind
```

## Next Steps

1. **Read the [Architecture](ARCHITECTURE.md)** to understand the internals
2. **Try the [Examples](examples/)** for more advanced usage
3. **Run [Benchmarks](benchmarks/)** to see performance metrics
4. **Read the [API Reference](README.md#api-reference)** for detailed documentation
5. **Contribute!** See [CONTRIBUTING.md](CONTRIBUTING.md)

## Troubleshooting

### Build fails with "CMake not found"
Install CMake 3.15+ via package manager or from https://cmake.org/

### Test fails with "Could not connect to broker"
Make sure the broker is running on port 9000. Check with `lsof -i :9000`

### Latency higher than expected
1. Run on Linux (macOS has higher syscall overhead)
2. Disable debug builds (use `make build` not `make debug`)
3. Pin threads to cores (`taskset -c 0 ./broker`)
4. Check CPU governor is set to "performance"

### Still stuck?
Open a GitHub issue with:
- OS and version
- Compiler and version
- Full error message
- Steps to reproduce

## Resources

- **Documentation**: https://github.com/yourusername/nanomq/wiki
- **Examples**: https://github.com/yourusername/nanomq/tree/main/examples
- **Issues**: https://github.com/yourusername/nanomq/issues
- **Discussions**: https://github.com/yourusername/nanomq/discussions

Happy messaging! 🚀

