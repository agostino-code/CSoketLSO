cmake_minimum_required(VERSION 3.16)
project(LSOProject C)

set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE c_files "${PROJECT_SOURCE_DIR}/src/*.c"
        "${PROJECT_SOURCE_DIR}/src/data/*.c"
        "${PROJECT_SOURCE_DIR}/src/json/*.c"
        )

file(GLOB_RECURSE h_files "${PROJECT_SOURCE_DIR}/src/*.h"
        "${PROJECT_SOURCE_DIR}/src/data/*.h"
        "${PROJECT_SOURCE_DIR}/src/json/*.h"
        )

add_executable(LSOProject
        ${c_files}
        ${h_files}
        )

add_subdirectory(lib/json-c)
target_link_libraries(LSOProject PRIVATE json-c)
add_subdirectory(lib/libpq-standalone)
target_link_libraries(LSOProject PRIVATE PostgreSQL::PostgreSQL)
