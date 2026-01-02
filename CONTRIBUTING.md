# Contributing to NanoMQ

Thank you for your interest in contributing to NanoMQ! This document provides guidelines for contributing to the project.

## Code of Conduct

Please be respectful and constructive in all interactions with the community.

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in GitHub Issues
2. If not, create a new issue with:
   - Clear description of the bug
   - Steps to reproduce
   - Expected vs actual behavior
   - Environment details (OS, compiler, version)
   - Relevant logs or error messages

### Suggesting Features

1. Open a GitHub issue with the `enhancement` label
2. Describe the feature and its use case
3. Explain why it would be valuable to users
4. Consider implementation details if applicable

### Submitting Pull Requests

1. **Fork the repository** and create a new branch
2. **Write code** following the style guidelines below
3. **Add tests** for new functionality
4. **Run all tests** to ensure nothing breaks
5. **Update documentation** if needed
6. **Commit with clear messages** describing your changes
7. **Open a Pull Request** with:
   - Description of changes
   - Link to related issues
   - Before/after performance metrics (if applicable)

## Development Setup

```bash
# Clone your fork
git clone https://github.com/yourusername/nanomq.git
cd nanomq

# Build debug version
make debug

# Run tests
make test

# Run benchmarks
make benchmark
```

## Code Style Guidelines

### C++ Style

- **Standard**: C++17
- **Naming**:
  - Classes: `PascalCase` (e.g., `SPSCQueue`)
  - Functions/methods: `snake_case` (e.g., `try_push`)
  - Variables: `snake_case` (e.g., `message_id`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_PAYLOAD_SIZE`)
  - Private members: trailing underscore (e.g., `head_`)
- **Formatting**: Use `clang-format` (run `make format`)
- **Comments**: Document public APIs with Doxygen-style comments

### Example

```cpp
/// Brief description of the function
/// @param item The item to push
/// @return true if successful, false if queue is full
bool try_push(const T& item) {
    const size_t head = head_.load(std::memory_order_relaxed);
    // Implementation...
}
```

## Testing Guidelines

1. **Unit Tests**: Test individual components in isolation
2. **Integration Tests**: Test interactions between components
3. **Performance Tests**: Verify latency/throughput targets
4. **Edge Cases**: Test boundary conditions and error handling

```cpp
TEST(SPSCQueueTest, DescriptiveName) {
    SPSCQueue<int, 16> queue;
    
    // Arrange
    int value = 42;
    
    // Act
    bool result = queue.try_push(value);
    
    // Assert
    EXPECT_TRUE(result);
}
```

## Performance Considerations

When contributing performance-critical code:

1. **Measure first**: Profile before optimizing
2. **Avoid allocations**: Minimize heap allocations in hot paths
3. **Cache-friendly**: Consider cache line alignment and access patterns
4. **Lock-free**: Prefer atomic operations over locks
5. **Benchmark**: Include before/after performance metrics

## Documentation

- Update README.md for user-facing changes
- Add inline comments for complex logic
- Include examples for new APIs
- Update architecture docs if design changes

## Commit Messages

Use clear, descriptive commit messages:

```
Add batch publish API for improved throughput

- Implement publish_batch() in Publisher class
- Add batch_publish example
- Include performance benchmark
- Update README with batch API documentation

Closes #42
```

## Review Process

1. All PRs require at least one approval
2. CI tests must pass
3. Code coverage should not decrease
4. Performance benchmarks should not regress
5. Documentation must be updated

## Questions?

Open a GitHub Discussion or reach out to the maintainers.

Thank you for contributing to NanoMQ! 🚀

