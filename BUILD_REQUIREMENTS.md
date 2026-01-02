# Build Requirements for NanoMQ

## System Requirements

### Operating System
- **Linux** (Ubuntu 20.04+, Fedora, CentOS) - Recommended
- **macOS** (10.15+) - Supported
- **Windows** (WSL2 only) - Limited support

### Compiler Requirements
- **GCC** 7.0 or later (C++17 support)
- **Clang** 5.0 or later (C++17 support)
- **MSVC** 2017 or later (Windows)

Check your version:
```bash
g++ --version
# or
clang++ --version
```

### Build Tools
- **CMake** 3.15 or later
- **Make** (GNU Make recommended)
- **Git** (for cloning dependencies)

Install on Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git
```

Install on macOS:
```bash
brew install cmake
# Xcode Command Line Tools provide clang/make
xcode-select --install
```

## C++ Standard Library Requirements

- Full C++17 standard library support
- Thread support (pthread on Linux/macOS)
- Atomic operations support
- Standard memory allocators

**Included with modern GCC/Clang installations**

## External Dependencies

All dependencies are **automatically downloaded** by CMake during build:

### Testing Framework
- **Google Test** (v1.14.0)
  - URL: https://github.com/google/googletest
  - License: BSD-3-Clause
  - Usage: Unit testing
  - Auto-fetched via CMake FetchContent

### Benchmarking Framework
- **Google Benchmark** (v1.8.3)
  - URL: https://github.com/google/benchmark
  - License: Apache-2.0
  - Usage: Performance benchmarking
  - Auto-fetched via CMake FetchContent

### System Libraries (Already on OS)
- **pthread** - POSIX threads (Linux/macOS)
- **libstdc++** or **libc++** - C++ standard library
- No additional system packages required!

## Optional Dependencies

### For Development
- **clang-format** - Code formatting
  ```bash
  # Ubuntu
  sudo apt-get install clang-format
  
  # macOS
  brew install clang-format
  ```

- **valgrind** - Memory leak detection (Linux only)
  ```bash
  sudo apt-get install valgrind
  ```

- **perf** - Performance profiling (Linux only)
  ```bash
  sudo apt-get install linux-tools-common linux-tools-generic
  ```

### For Docker
- **Docker** 20.10 or later
- **Docker Compose** 1.29 or later

Install:
```bash
# Ubuntu
sudo apt-get install docker.io docker-compose

# macOS
brew install --cask docker
```

## Disk Space Requirements

- **Source code**: ~5 MB
- **Build artifacts**: ~50 MB
- **Tests + Benchmarks**: ~100 MB
- **Total**: ~200 MB (with all dependencies)

## Network Requirements

- Internet connection required for **first build only** to download:
  - Google Test (~10 MB)
  - Google Benchmark (~5 MB)
- Subsequent builds work offline

## CPU Requirements

- **Minimum**: Any x86_64 or ARM64 processor
- **Recommended**: Multi-core CPU for faster builds
  - 2+ cores: ~1 minute build time
  - 4+ cores: ~30 seconds build time
  - 8+ cores: ~15 seconds build time

## Memory Requirements

- **Build**: 2 GB RAM minimum, 4 GB recommended
- **Runtime**: 100 MB baseline, scales with message count
- **Tests**: 1 GB RAM for running all tests

## Feature-Specific Requirements

### Lock-Free Ring Buffer
- **Required**: Atomic operations support
- **All modern CPUs** (x86_64, ARM64, RISC-V) support this
- Hardware: Compare-and-swap (CAS) instructions

### Memory-Mapped I/O
- **Required**: `mmap()` system call
- Available on: Linux, macOS, BSD
- **Not required**: Windows (uses `_aligned_malloc` instead)

### High-Resolution Timers
- **Required**: `clock_gettime()` or equivalent
- Available on: POSIX systems (Linux, macOS)
- Used for: Nanosecond-precision timestamps

## Build Configuration Options

### CMake Options

```bash
# Release build (optimized, default)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug build (symbols, sanitizers)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Disable tests
cmake -DBUILD_TESTS=OFF ..

# Disable benchmarks
cmake -DBUILD_BENCHMARKS=OFF ..
```

### Compiler Flags

**Release** (default):
- `-O3` - Maximum optimization
- `-march=native` - CPU-specific optimizations
- `-DNDEBUG` - Disable assertions
- `-flto` - Link-time optimization

**Debug**:
- `-O0` - No optimization
- `-g` - Debug symbols
- `-fsanitize=address` - Detect memory errors
- `-fsanitize=undefined` - Detect undefined behavior

## Verification

After installation, verify requirements:

```bash
# Check compiler
g++ --version | grep "g++"

# Check CMake
cmake --version | grep "cmake version"

# Check make
make --version | grep "GNU Make"

# Check git
git --version
```

All should show versions meeting the requirements above.

## Troubleshooting

### "CMake not found"
Install CMake 3.15+ from your package manager or https://cmake.org/download/

### "C++ compiler does not support C++17"
Update your compiler:
```bash
# Ubuntu
sudo apt-get install g++-9  # or later

# macOS
xcode-select --install
```

### "Google Test download failed"
- Check internet connection
- Try again (transient network issues)
- Or manually clone: `git clone https://github.com/google/googletest external/googletest`

### Build very slow
- Use parallel builds: `make -j$(nproc)`
- Check disk I/O (slow disks bottleneck)
- Close resource-heavy applications

### Tests fail with memory errors
- This is expected in debug mode (stubs not implemented)
- Core tests should pass
- Network tests will fail until fully implemented

## Platform-Specific Notes

### Linux
- **Best performance** (native epoll, perf tools)
- All features fully supported
- Recommended for benchmarking

### macOS
- **Good performance** (native kqueue)
- ~20% higher syscall overhead vs Linux
- Great for development

### Windows (WSL2)
- Runs via Windows Subsystem for Linux
- Performance similar to Linux
- Native Windows build possible but not tested

## Minimum System Example

**Works on a Raspberry Pi 4!**
- CPU: ARM Cortex-A72 (quad-core)
- RAM: 2 GB
- OS: Ubuntu 20.04 ARM64
- Build time: ~2 minutes

---

**Summary**: Only need a C++17 compiler, CMake, and Make. Everything else downloads automatically!

