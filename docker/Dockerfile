FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . /app

# Build NanoMQ
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Expose broker port
EXPOSE 9000

# Create data directory
RUN mkdir -p /data

# Run broker by default
CMD ["./build/nanomq-broker", "--port", "9000", "--data-dir", "/data"]

