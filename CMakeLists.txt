cmake_minimum_required(VERSION 3.26)
project(LSOProject C)

set(CMAKE_C_STANDARD 11)

set(c_files
        globals.c
        handle_client.c
        handle_player.c
        handle_room.c
        ./data/room.c
        ./data/player.c
        )
add_executable(LSOProject main.c ${c_files})
add_executable(tests test.c)
enable_testing()
add_test(NAME MyTest COMMAND tests)

add_subdirectory(yyjson-0.8.0)
target_link_libraries(LSOProject PRIVATE yyjson)
set(YYJSON_DISABLE_NON_STANDARD ON CACHE INTERNAL "")
add_subdirectory(libpq-standalone)
target_link_libraries(LSOProject PRIVATE PostgreSQL::PostgreSQL)