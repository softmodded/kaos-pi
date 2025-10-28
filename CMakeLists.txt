cmake_minimum_required(VERSION 3.10)
project(kaos-pi C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# Find required packages
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    src/main.c
    src/usb_gadget.c
    src/portal.c
    src/web_server.c
    src/crypto/skylander_crypt.c
    src/crypto/rijndael.c
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/crypto
)

# Create executable
add_executable(kaos-pi ${SOURCES})

# Link libraries
target_link_libraries(kaos-pi
    ${CMAKE_THREAD_LIBS_INIT}
    m
)

# Installation
install(TARGETS kaos-pi DESTINATION /usr/local/bin)

# Create skylanders directory on install
install(DIRECTORY DESTINATION /var/lib/kaos-pi/skylanders)

# Print build info
message(STATUS "Building KAOS-Pi Portal Emulator")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
