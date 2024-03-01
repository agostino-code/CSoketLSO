cmake_minimum_required(VERSION 3.16)
project(LSOProject C)

set(CMAKE_C_STANDARD 11)

set(c_files
        globals.c
        handle_client.c
        handle_player.c
        handle_room.c
        ./data/room.c
        ./data/player.c
        ./json/formatter.c
        )
add_executable(LSOProject main.c ${c_files})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBBSD REQUIRED libbsd)
include_directories(${LIBBSD_INCLUDE_DIRS})

add_subdirectory(json-c)
target_link_libraries(LSOProject PRIVATE json-c ${LIBBSD_LIBRARIES})
add_subdirectory(libpq-standalone)
target_link_libraries(LSOProject PRIVATE PostgreSQL::PostgreSQL)