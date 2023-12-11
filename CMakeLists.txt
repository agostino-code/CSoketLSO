cmake_minimum_required(VERSION 3.26)
project(LSOProject C)

set(CMAKE_C_STANDARD 11)

add_executable(LSOProject main.c)

add_subdirectory(yyjson-0.8.0)
target_link_libraries(LSOProject PRIVATE yyjson)
set(YYJSON_DISABLE_NON_STANDARD ON CACHE INTERNAL "")
add_subdirectory(libpq-standalone)
target_link_libraries(LSOProject PRIVATE PostgreSQL::PostgreSQL)