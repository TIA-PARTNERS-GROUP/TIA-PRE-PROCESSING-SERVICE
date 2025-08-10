# Set the minimum required version of CMake and define the project
cmake_minimum_required(VERSION 3.16)
project(MemgraphSyncService CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Define where the final executable will be placed ---
# This matches the path used in the Dockerfile's COPY command
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Find Pre-Installed Dependencies using PkgConfig ---
# Finds libraries provided by the Dockerfile's apt-get install
find_package(PkgConfig REQUIRED)
pkg_check_modules(RdKafka REQUIRED rdkafka++)
pkg_check_modules(MariaDB REQUIRED libmariadb)
pkg_check_modules(LZ4 REQUIRED liblz4)

# --- Download Dependencies using ExternalProject ---

# Use ExternalProject to download and build the mgclient library.
include(ExternalProject)
ExternalProject_Add(mgclient-proj
    GIT_REPOSITORY https://github.com/memgraph/mgclient.git
    GIT_TAG v1.4.0 # Using a specific tag for reproducibility
    # Define where the project will be built and installed internally
    PREFIX ${CMAKE_BINARY_DIR}/mgclient
    # CMake arguments for the external project
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DBUILD_CPP_BINDINGS=ON
    # Disable logging for a cleaner build output
    LOG_CONFIGURE 1
    LOG_BUILD 1
    LOG_INSTALL 1
)

# Create an IMPORTED target for the mgclient library that we just built.
add_library(mgclient-lib SHARED IMPORTED)
set_property(TARGET mgclient-lib PROPERTY IMPORTED_LOCATION
    ${CMAKE_BINARY_DIR}/mgclient/lib/libmgclient.so)
# Ensure mgclient is built before we try to link to it
add_dependencies(mgclient-lib mgclient-proj)

# Use ExternalProject to download the header-only nlohmann/json library.
ExternalProject_Add(json-proj
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2 # Using a specific tag for reproducibility
    PREFIX ${CMAKE_BINARY_DIR}/json
    # No build/install steps are needed, just download and place the headers
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

# Create an INTERFACE target to provide the include directory for nlohmann/json.
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
    ${CMAKE_BINARY_DIR}/json/src/json-proj/include)
# Ensure json is downloaded before we try to use it
add_dependencies(nlohmann_json json-proj)


# --- Define the Executable Target ---

# This is the key fix: Point to the source file inside the 'src' directory.
add_executable(memgraph-sync-service
  src/main.cpp
  src/kafka_client.cpp
  src/memgraph_client.cpp
  src/message_handler.cpp
)

# The Dockerfile expects the final binary to be named 'main'.
set_target_properties(memgraph-sync-service PROPERTIES OUTPUT_NAME "main")

# --- Link All Libraries to the Executable ---
target_include_directories(memgraph-sync-service PRIVATE
    # Add include directory for the mgclient we built
    ${CMAKE_BINARY_DIR}/mgclient/include
)

target_link_libraries(memgraph-sync-service PRIVATE
    # Link our custom-built libraries
    mgclient-lib
    nlohmann_json
    # Link libraries found by PkgConfig
    ${RdKafka_LIBRARIES}
    ${MariaDB_LIBRARIES}
    ${LZ4_LIBRARIES}
    # Link other system libraries required by the dependencies
    ssl
    crypto
    sasl2
    z
)

message(STATUS "Build configuration complete.")
