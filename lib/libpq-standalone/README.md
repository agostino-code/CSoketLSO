# libpq-standalone

PostgreSQL client library libpq as a standalone project with CMake build scripts


This project might be something for you, if you
- want to embed the PostgreSQL client library into your own project, so that the users of your project won't need to install PostgreSQL as an extra dependency,
- use CMake,
- want to link statically against libpq,
- want to build just libpq as fast as possible (without the huge server part).

## Design goals

PostgreSQL comes with automake/autoconf scripts which allow for easy
compilation on Linux and Unix-like systems. But the build process
on Windows requires Perl, which brings an additional requirement.

For programs requiring just linking against the PostgreSQL client library this means additional effort. It is the goal of this project to reduce this effort by providing the following features:

- **Trivial integration**. Make it possible to easily integrate the PostgreSQL client library into ones own project with minimal additional dependencies
- **CMake**. Build with CMake, provide a CMake target for linking against libpq, compatilbe with `find_package(PostgreSQL)`
- **ABI compatibility**. Build "libpq" as a static library with exactly the same ABI and C compiler and flags as the rest of your project
- **Portability**. Builds on most commonly used platforms (Linux, Mac, Windows, ...)
- **Up to date**. Currently this library contains a copy of the client library sources from the PostgreSQL Version 15 branch. (Check out the `REL_12_STABLE` to 
`REL_14_STABLE` branches of this project for older PostgreSQL versions).


## Integration

```
cd my_procject
cp -r ../libpq-standalone/libpq .
```

To integrate this library into your own project, check out `libpq-standalone` and copy the `libpq` subdirectory to some place into your project's source tree, e.g. into a `third-party` or `lib` subdirectory.

Then, edit your `CMakeLists.txt` and replace the call to `find_package(PostgreSQL)` with an `add_subdirectory(libpq)`.

```cmake
add_subdirectory(libpq)
add_executable(my_program ...)
...
target_link_libraries(my_program PRIVATE PostgreSQL::PostgreSQL)
```

.. or make it possible to still use an installed version of libpq ...

```cmake
option(USE_SYSTEM_LIBPQ "Use installed version of PostgreSQL client library instead of included one" OFF)
if (USE_SYSTEM_LIBPQ)
    find_package(PostgreSQL)
else()
    add_subdirectory(libpq)
endif()
```

If you do not want to distribute libpq with you project sources but still want
an integrated build process you can use CMake's FetchContent function. Just
include the following snippet into your CMakeLists.txt file.

```cmake
include(FetchContent)
FetchContent_Declare(libpq
    GIT_REPOSITORY https://gitlab.com/sabelka/libpq-standalone
    SOURCE_SUBDIR libpq)
FetchContent_MakeAvailable(libpq)
```

## Supported Compilers

Tested with the following compilers:

- gcc on Linux (versions 9, 10, 11 and 12)
- clang on Linux (10, 11, 12)
- MSCV 14.25 (Visual Studio 2019)
- Apple clang version 11.0.3 (clang-1103.0.32.59)

... and probably more.  I would be happy to learn about other compilers/versions.


## PostgreSQL

This project contains a copy of the source files required to build the PostgreSQL client library from the official PostgreSQL repository at git://git.postgresql.org/git/postgresql.git (PostgreSQL version 15).
In the scripts subdirectory there is a shell script which can be used to automatically update the sources from a local PostgreSQL repository clone.


## License

The sources files which have been copied from the PostgreSQL distribution are released under the PostgreSQL License. See https://www.postgresql.org/about/licence/ for the exact terms of this license. 
For the remainder of the content in this repository (scripts, CMake files, documentation, etc., the PostgreSQL License shall apply as well.
