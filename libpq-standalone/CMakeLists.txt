cmake_minimum_required(VERSION 3.15...3.23)

project(libpq-standalone CXX)
#include(cmake/StandardProjectSettings.cmake)

#add_library(project_options INTERFACE)
#target_compile_features(project_options INTERFACE cxx_std_17)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
#add_library(project_warnings INTERFACE)

# standard compiler warnings
#include(cmake/CompilerWarnings.cmake)
#set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
#include(cmake/Sanitizers.cmake)
#enable_sanitizers(project_options)

# allow for static analysis options
#include(cmake/StaticAnalyzers.cmake)

#option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)
#option(ENABLE_TESTING "Enable Test Builds" ON)

#if(ENABLE_TESTING)
#  enable_testing()
#  message(
#    "Building Tests. Be sure to check out test/constexpr_tests for constexpr testing"
#  )
  add_subdirectory(test)
#endif()

add_subdirectory(libpq)
