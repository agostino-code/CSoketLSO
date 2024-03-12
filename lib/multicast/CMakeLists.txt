cmake_minimum_required(VERSION 3.16)
project(multicast)

set(CMAKE_C_STANDARD 11)

# Add your source files here
set(SOURCES
    multicast_send.c
    multicast_recv.c
)

# Add your header files here
set(HEADERS
    multicast.h
    multicast_defs.h
)

# Create the library
add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})