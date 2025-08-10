# =========================================================================
# Stage 1: The Builder
# This stage installs all development tools and dependencies, builds the
# application, and prepares the final binary for the runtime stage.
# =========================================================================
FROM ubuntu:latest AS builder

# Set the environment variable to prevent interactive prompts during package installation.
ENV DEBIAN_FRONTEND=noninteractive

# Install all necessary dependencies for building the C++ application.
# This includes the compiler (g++), build systems (make, cmake), version control (git),
# and all required development libraries (-dev packages) for Kafka, MariaDB, SSL, etc.
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
  libsasl2-dev \
  librdkafka-dev \
  ca-certificates \
  curl \
  --no-install-recommends

# --- Build and install mgclient from source ---
# NOTE: This step is an alternative to using CMake's ExternalProject. It builds
# the dependency once and installs it system-wide within the builder image.
WORKDIR /tmp
RUN git clone https://github.com/memgraph/mgclient.git && \
  cd mgclient && \
  mkdir build && cd build && \
  # Configure mgclient to be installed into a standard system location.
  cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && \
  # Compile the project using all available CPU cores for speed.
  make -j$(nproc) && \
  # Install the compiled libraries and headers into /usr/local.
  make install

# --- Download the nlohmann/json header-only library ---
# For header-only libraries, a simple download is often easier than a full build process.
WORKDIR /app
RUN mkdir -p include/nlohmann && \
  curl -L https://github.com/nlohmann/json/releases/latest/download/json.hpp -o include/nlohmann/json.hpp

# Copy the application's source code (including CMakeLists.txt) into the builder stage.
COPY . .

# --- Use CMake to configure and build our main application ---
# This process uses the CMakeLists.txt file from our source code.
RUN mkdir build && \
  cd build && \
  # Generate the Makefiles from our CMakeLists.txt blueprint.
  cmake .. && \
  # Compile our application using all available CPU cores.
  make -j$(nproc)


# =========================================================================
# Stage 2: The Final Runtime Image
# This stage creates a minimal, secure image containing only the compiled
# application and its runtime dependencies. It does not contain any build tools.
# =========================================================================
FROM ubuntu:latest

# Install only the required RUNTIME libraries, not the -dev packages.
# These are the shared objects (.so files) the binary needs to run.
RUN apt-get update && apt-get install -y \
  librdkafka++1 \
  libmariadb3 \
  openssl \
  # Clean up apt cache to reduce the final image size.
  && rm -rf /var/lib/apt/lists/*

# Copy the mgclient shared library that we built from source in the 'builder' stage.
# This is a key part of the multi-stage build pattern.
COPY --from=builder /usr/local/lib/libmgclient.so* /usr/lib/

# After adding a new shared library, refresh the dynamic linker's cache
# so the operating system can find it when our application starts.
RUN ldconfig

# --- Security Best Practice: Create and use a non-root user ---
RUN useradd -m myapp
USER myapp
WORKDIR /home/myapp

# Copy the compiled executable from the 'builder' stage into the final image.
# The destination path is '/app/build/bin/main' as defined by our CMakeLists.txt.
COPY --from=builder /app/build/bin/main .

# Set the default command to run when a container is started from this image.
CMD ["./main"]
