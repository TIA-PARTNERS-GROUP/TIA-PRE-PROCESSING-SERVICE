FROM ubuntu:latest as builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install all build dependencies using apt-get
RUN apt-get update && apt-get install -y \
  g++ \
  make \
  git \
  cmake \
  openssl \
  libssl-dev \
  pkg-config \
  libmariadb-dev \
  ca-certificates \
  wget \
  gnupg \
  --no-install-recommends

RUN apt-get install -y git cmake make g++ libssl-dev && \
  git clone --recursive https://github.com/memgraph/mgclient.git /tmp/mgclient && \
  cd /tmp/mgclient && \
  mkdir build && cd build && \
  cmake .. && \
  make -j$(nproc) && \
  make install && \
  ldconfig

# --- Now, build your application ---
WORKDIR /app
COPY . .
# This 'make' command will succeed because all dependencies are installed
RUN make

# Stage 2: The Final Image
FROM ubuntu:latest

# Create a non-root user for security
RUN useradd -m myapp

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
  libssl3 \
  ca-certificates \
  && rm -rf /var/lib/apt/lists/*

# Copy the required runtime libraries from the builder stage
COPY --from=builder /usr/lib/x86_64-linux-gnu/libmariadb.so.3 /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libmgclient.so* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/libmgclient.so* /usr/lib/ 

# Run ldconfig to update the dynamic linker cache in the final image
RUN ldconfig

USER myapp
WORKDIR /home/myapp

# Copy your compiled application from the builder stage
COPY --from=builder /app/bin/main /usr/local/bin/main

# Set the default command
CMD ["main"]
