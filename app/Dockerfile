# Stage 1: The Builder
FROM ubuntu:latest AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install all build dependencies
RUN apt-get update && apt-get install -y \
  g++ \
  make \
  git \
  cmake \
  clang \
  openssl \
  build-essential \
  libssl-dev \
  pkg-config \
  libmariadb-dev \
  liblz4-dev \
  zlib1g-dev \
  libssl-dev \
  libsasl2-dev \
  librdkafka-dev \
  ca-certificates \
  curl \
  --no-install-recommends

# Build and install mgclient
WORKDIR /tmp
RUN git clone https://github.com/memgraph/mgclient.git && \
  cd mgclient && \
  mkdir build && cd build && \
  cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && \
  make -j$(nproc) && \
  make install

# Download nlohmann/json header
WORKDIR /app
RUN mkdir -p include/nlohmann && \
  curl -L https://github.com/nlohmann/json/releases/latest/download/json.hpp -o include/nlohmann/json.hpp

# Copy application source (including CMakeLists.txt)
COPY . .

# --- UPDATED: Use CMake to configure and build the project ---
RUN mkdir build && \
  cd build && \
  cmake .. && \
  make -j$(nproc)

# Stage 2: The Final Runtime Image
FROM ubuntu:latest

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
  librdkafka++1 \
  libmariadb3 \
  openssl \
  && rm -rf /var/lib/apt/lists/*

# Copy mgclient shared libraries explicitly from the builder stage
COPY --from=builder /usr/local/lib/libmgclient.so* /usr/lib/

# Refresh the dynamic linker cache to recognize the new libraries
RUN ldconfig

# Create non-root user and set working dir for security
RUN useradd -m myapp
USER myapp
WORKDIR /home/myapp

# Copy the built binary from the builder stage
# The CMakeLists.txt places the executable in /app/bin/
COPY --from=builder /app/build/bin/main .

CMD ["./main"]
