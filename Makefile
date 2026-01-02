.PHONY: all build clean test benchmark run-broker install format

# Default target
all: build

# Create build directory and compile
build:
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)
	@echo "✓ Build complete"

# Debug build with sanitizers
debug:
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$(nproc)
	@echo "✓ Debug build complete"

# Run tests
test: build
	@cd build && ctest --output-on-failure
	@echo "✓ All tests passed"

# Run benchmarks
benchmark: build
	@echo "=== Latency Benchmark ==="
	@./build/bench_latency
	@echo "\n=== Throughput Benchmark ==="
	@./build/bench_throughput
	@echo "\n=== CPU Benchmark ==="
	@./build/bench_cpu

# Run broker
run-broker: build
	@./build/nanomq-broker --port 9000 --data-dir ./data

# Install
install: build
	@cd build && sudo make install
	@echo "✓ Installation complete"

# Clean build artifacts
clean:
	@rm -rf build data
	@echo "✓ Cleaned"

# Format code with clang-format
format:
	@find include src tests benchmarks examples -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
	@echo "✓ Code formatted"

# Docker build
docker-build:
	@docker build -t nanomq:latest -f docker/Dockerfile .
	@echo "✓ Docker image built"

# Docker run
docker-run:
	@docker-compose -f docker/docker-compose.yml up -d
	@echo "✓ NanoMQ running in Docker"

# Performance profiling
profile: build
	@perf record -g ./build/bench_throughput
	@perf report
	@echo "✓ Profiling complete"

# Memory leak check
valgrind: debug
	@valgrind --leak-check=full --show-leak-kinds=all ./build/test_ring_buffer
	@echo "✓ Valgrind check complete"

# Help
help:
	@echo "NanoMQ Build System"
	@echo "==================="
	@echo "make build        - Build release version"
	@echo "make debug        - Build debug version with sanitizers"
	@echo "make test         - Run all tests"
	@echo "make benchmark    - Run performance benchmarks"
	@echo "make run-broker   - Start NanoMQ broker"
	@echo "make install      - Install system-wide"
	@echo "make clean        - Remove build artifacts"
	@echo "make format       - Format code with clang-format"
	@echo "make docker-build - Build Docker image"
	@echo "make docker-run   - Run in Docker"
	@echo "make profile      - Profile with perf"
	@echo "make valgrind     - Check memory leaks"

