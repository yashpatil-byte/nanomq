#pragma once

// Main NanoMQ header - include this to use the library

#include "nanomq/message.hpp"
#include "nanomq/publisher.hpp"
#include "nanomq/subscriber.hpp"
#include "nanomq/queue.hpp"

namespace nanomq {

// Version information
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

inline const char* version() {
    return "1.0.0";
}

// Initialize NanoMQ library (call once at startup)
void init();

// Shutdown NanoMQ library (call once at exit)
void shutdown();

}  // namespace nanomq

