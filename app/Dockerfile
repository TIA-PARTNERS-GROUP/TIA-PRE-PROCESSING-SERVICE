# Stage 1: The Builder
FROM ubuntu:latest AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install all build dependencies
RUN apt-get update && apt-get install -y \
  g++ \
  make \
  git \
  cmake \
  openssl \
  libssl-dev \
  pkg-config \
  libmariadb-dev \
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

# Copy application source and build
COPY . .
RUN make

# Stage 2: The Final Runtime Image
FROM ubuntu:latest

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
  librdkafka++1 \
  libmariadb3 \
  openssl \
  && rm -rf /var/lib/apt/lists/*

# Copy mgclient shared libraries explicitly
COPY --from=builder /usr/local/lib/libmgclient.so* /usr/lib/

# Copy MariaDB lib in case it's needed
COPY --from=builder /usr/lib/x86_64-linux-gnu/libmariadb.so.3 /usr/lib/x86_64-linux-gnu/

# Refresh the dynamic linker cache
RUN ldconfig

# Optional: Set LD_LIBRARY_PATH to help the linker
ENV LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH

# Create non-root user and set working dir
RUN useradd -m myapp
USER myapp
WORKDIR /home/myapp

# Copy built binary
COPY --from=builder /app/bin/main .

# Confirm the binary can find all its linked libraries (for debugging)
# RUN ldd ./main  # Uncomment this line to debug inside image build

CMD ["./main"]
