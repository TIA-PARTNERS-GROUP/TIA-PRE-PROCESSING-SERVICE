# --- Project and CMake Version Setup ---
# Specifies the minimum version of CMake required to build this project.
cmake_minimum_required(VERSION 3.16)
# Defines the project name and specifies that it's a C++ (CXX) project.
project(MemgraphSyncService CXX)

# --- C++ Standard Configuration ---
# Sets the C++ standard to C++17 for the entire project.
set(CMAKE_CXX_STANDARD 17)
# Enforces that the specified C++ standard is used; CMake will error if the compiler doesn't support it.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disables compiler-specific extensions (e.g., -std=gnu++17) for better portability.
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Output Directory Configuration ---
# Specifies that the final executable should be placed in the 'bin' directory within the build folder.
# This keeps the build directory organized and matches the path used in the Dockerfile's COPY command.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Find Pre-Installed System Dependencies ---
# PkgConfig is a tool to help find compiler and linker flags for libraries installed on the system.
find_package(PkgConfig REQUIRED)
# Use PkgConfig to find the rdkafka++ library and its dependencies.
pkg_check_modules(RdKafka REQUIRED rdkafka++)
# Use PkgConfig to find the MariaDB client library.
pkg_check_modules(MariaDB REQUIRED libmariadb)
# Use PkgConfig to find the LZ4 compression library.
pkg_check_modules(LZ4 REQUIRED liblz4)

# --- Download and Build Dependencies from Source ---

# Use CMake's ExternalProject module to manage dependencies that are not pre-installed.
include(ExternalProject)

# --- mgclient (Memgraph C/C++ Client) ---
# Defines an external project to download, configure, and build the mgclient library from its Git repository.
ExternalProject_Add(mgclient-proj
    # The location of the source code repository.
    GIT_REPOSITORY https://github.com/memgraph/mgclient.git
    # A specific version tag is used to ensure the build is reproducible and stable.
    GIT_TAG v1.4.0
    # Defines a directory prefix where the download, build, and install steps will occur.
    PREFIX ${CMAKE_BINARY_DIR}/mgclient
    # Arguments passed to the external project's own CMake configuration step.
    # <INSTALL_DIR> is a placeholder that points to the install directory within the PREFIX.
    # We enable the C++ bindings, which are required for this project.
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DBUILD_CPP_BINDINGS=ON
    # Disable verbose logging from the external build process for a cleaner output.
    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_INSTALL 1
)

# Create a CMake target representing the mgclient shared library we just built.
# 'SHARED IMPORTED' means this target represents a pre-existing shared library file (.so).
add_library(mgclient-lib SHARED IMPORTED)
# Tell CMake where to find the actual library file created by the ExternalProject build.
set_property(TARGET mgclient-lib PROPERTY IMPORTED_LOCATION
    ${CMAKE_BINARY_DIR}/mgclient/lib/libmgclient.so)
# Create a dependency to ensure that 'mgclient-proj' is fully built before any target that links to 'mgclient-lib' is processed.
add_dependencies(mgclient-lib mgclient-proj)


# --- nlohmann/json (JSON for Modern C++) ---
# Defines an external project to download the nlohmann/json library.
ExternalProject_Add(json-proj
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2 # Use a specific version for reproducibility.
    PREFIX ${CMAKE_BINARY_DIR}/json
    # Since this is a header-only library, no configuration, build, or install steps are needed.
    # We provide empty commands to override the defaults.
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

# Create an INTERFACE library target for nlohmann/json.
# INTERFACE libraries don't compile into anything themselves; they are used to propagate usage requirements (like include directories) to targets that link them.
add_library(nlohmann_json INTERFACE)
# Specify that any target linking against nlohmann_json should have its include path updated to find the downloaded headers.
target_include_directories(nlohmann_json INTERFACE
    ${CMAKE_BINARY_DIR}/json/src/json-proj/include)
# Ensure the json-proj download is complete before any target tries to use nlohmann_json.
add_dependencies(nlohmann_json json-proj)


# --- Define the Main Executable Target ---

# Creates the executable target named 'memgraph-sync-service' from the specified source files.
add_executable(memgraph-sync-service
  src/main.cpp
  src/kafka_client.cpp
  src/memgraph_client.cpp
  src/message_handler.cpp
)

# Renames the output binary from 'memgraph-sync-service' to 'main'.
# This is done because the Dockerfile specifically expects to find a binary named 'main'.
set_target_properties(memgraph-sync-service PROPERTIES OUTPUT_NAME "main")

# --- Link All Libraries and Include Directories to the Executable ---

# Adds include directories to our executable's build configuration.
target_include_directories(memgraph-sync-service PRIVATE
    # Add the include directory for the mgclient C++ bindings we built.
    ${CMAKE_BINARY_DIR}/mgclient/include
)

# Links all necessary libraries to our executable.
target_link_libraries(memgraph-sync-service PRIVATE
    # Link our custom-built libraries from ExternalProject.
    mgclient-lib
    nlohmann_json

    # Link the libraries found by PkgConfig. The variables are set by the pkg_check_modules calls.
    ${RdKafka_LIBRARIES}
    ${MariaDB_LIBRARIES}
    ${LZ4_LIBRARIES}

    # Link other system libraries that are dependencies of our dependencies (e.g., rdkafka++ needs ssl, crypto).
    ssl
    crypto
    sasl2
    z
)

# Prints a status message to the console upon successful configuration.
message(STATUS "Build configuration complete.")
