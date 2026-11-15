include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

CPMAddPackage(
  NAME CLI11
  VERSION 2.7.2
  URL https://codeload.github.com/CLIUtils/CLI11/tar.gz/refs/tags/v2.7.2
  URL_HASH SHA256=46eef3101da70852ec7af026e09d485ccee81813331c8c6052d39344443b83da
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  OPTIONS
    "CLI11_BUILD_TESTS OFF"
    "CLI11_BUILD_EXAMPLES OFF"
    "CLI11_BUILD_DOCS OFF"
    "CLI11_PRECOMPILED OFF"
    "CLI11_MODULES ON"
)

set(RANGEQUERY_BENCHMARK_VERSION 1.9.4)
CPMAddPackage(
  NAME benchmark
  VERSION ${RANGEQUERY_BENCHMARK_VERSION}
  URL https://codeload.github.com/google/benchmark/tar.gz/refs/tags/v${RANGEQUERY_BENCHMARK_VERSION}
  URL_HASH SHA256=b334658edd35efcf06a99d9be21e4e93e092bd5f95074c1673d5c8705d95c104
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  OPTIONS
    "BENCHMARK_ENABLE_TESTING OFF"
    "BENCHMARK_ENABLE_INSTALL OFF"
    "BENCHMARK_ENABLE_WERROR OFF"
    "BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF"
)

# Clang 23 diagnoses Benchmark's __COUNTER__ probe as a C2y extension;
# its own -pedantic-errors otherwise stops the build.
if(benchmark_ADDED AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 23)
  target_compile_options(benchmark PRIVATE -Wno-c2y-extensions)
  target_compile_options(benchmark_main PRIVATE -Wno-c2y-extensions)
endif()

if(RANGEQUERY_BUILD_TESTS AND BUILD_TESTING)
  CPMAddPackage(
    NAME googletest
    VERSION 1.18.0
    URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.18.0
    URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    OPTIONS
      "BUILD_GMOCK ON"
      "gtest_build_tests OFF"
      "INSTALL_GTEST OFF"
  )
endif()
