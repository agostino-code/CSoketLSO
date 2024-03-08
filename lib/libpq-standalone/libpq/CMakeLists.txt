project(postrgresql_libpq VERSION 15.4)
find_package(Threads REQUIRED)

option(LIBPQ_WITH_OPENSSL "Build with OpenSSL" FALSE)
option(LIBPQ_WITH_GSSAPI "Build with GSSAPI support" FALSE)

if (LIBPQ_WITH_OPENSSL)
    find_package(OpenSSL REQUIRED)
endif()

if (WIN32)
    set(LIBPQ_CONFIG "win32")
elseif (APPLE)
    set(LIBPQ_CONFIG "macos")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
   set(LIBPQ_CONFIG "linux_x64")
else()
   message(SEND_ERROR "Unsupported system")
endif()

math(EXPR PG_VERSION_NUM "10000 * ${PROJECT_VERSION_MAJOR} + ${PROJECT_VERSION_MINOR}")

message(STATUS "Configuring for ${LIBPQ_CONFIG}")

configure_file(pg_config.h.in pg_config.h)

add_library(pq STATIC
    fe-auth-scram.c
    fe-connect.c
    fe-exec.c
    fe-lobj.c
    fe-misc.c
    fe-print.c
    fe-protocol3.c
    fe-secure.c
    fe-trace.c
    legacy-pqsignal.c
    libpq-events.c
    pqexpbuffer.c
    fe-auth.c

    # common
    base64.c
    cryptohash.c
    encnames.c
    hmac.c
    ip.c
    link-canary.c
    md5.c
    md5_common.c
    saslprep.c
    sha2.c
    sha1.c
    scram-common.c
    string.c
    unicode_norm.c
    wchar.c

    # port
    chklocale.c
    explicit_bzero.c
    inet_net_ntop.c
    noblock.c
    pg_strong_random.c
    pgstrcasecmp.c
    snprintf.c
    strerror.c
    thread.c
    #
    getpeereid.c
)

if (NOT APPLE)
    target_sources(pq PRIVATE
        strlcpy.c
    )
endif()

if (MSVC)
    target_include_directories(pq PRIVATE win32_msvc)
endif ()

if (WIN32)
    target_include_directories(pq PRIVATE win32)
    target_sources(pq PRIVATE
	dirmod.c
	getaddrinfo.c
	inet_aton.c
	open.c
	pgsleep.c
	win32.c
	win32error.c
	win32setlocale.c
    )
    if  (NOT CMAKE_USE_PTHREADS_INIT)
        target_sources(pq PRIVATE pthread-win32.c)
    endif ()
    target_link_libraries(pq PUBLIC wsock32 ws2_32 secur32)
endif()

if (LIBPQ_WITH_OPENSSL)
    target_sources(pq PRIVATE
        fe-secure-common.c
        fe-secure-openssl.c
    )
    target_link_libraries(pq PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto)
endif ()

target_compile_definitions(pq PRIVATE
    _REENTRANT
    _THREAD_SAFE
    _POSIX_PTHREAD_SEMANTICS
    FRONTEND
    _GNU_SOURCE
)

target_link_libraries(pq PUBLIC Threads::Threads)

target_include_directories(pq SYSTEM INTERFACE include)
target_include_directories(pq PRIVATE . include ${CMAKE_CURRENT_BINARY_DIR})

add_library(PostgreSQL::pq ALIAS pq)
add_library(PostgreSQL::PostgreSQL ALIAS pq)
