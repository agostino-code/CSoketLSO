project(test)

add_executable(libpq_version
    libpq_version.c
)

target_link_libraries(libpq_version PRIVATE PostgreSQL::pq)

