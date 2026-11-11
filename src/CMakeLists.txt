add_library(rangequery_modules)

target_sources(rangequery_modules
  PUBLIC
    FILE_SET CXX_MODULES FILES
      structures/kind.cppm
      config/config.cppm
      structures/contracts.cppm
      structures/ordered_vector/ordered_vector.cppm
      structures/std_set/std_set.cppm
      structures/avl/avl.cppm
      structures/treap/treap.cppm
      structures/factory/factory.cppm
  PRIVATE
      config/config.cpp
      structures/avl/avl.cpp
      structures/treap/treap.cpp
      structures/factory/factory.cpp
)

target_compile_features(rangequery_modules PUBLIC cxx_std_23)

find_package(CLI11 2.7.2 EXACT CONFIG QUIET)
if(NOT CLI11_FOUND)
  include(FetchContent)
  set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(CLI11_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(CLI11_PRECOMPILED OFF CACHE BOOL "" FORCE)
  set(CLI11_MODULES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(CLI11
    URL https://codeload.github.com/CLIUtils/CLI11/tar.gz/refs/tags/v2.7.2
    URL_HASH SHA256=46eef3101da70852ec7af026e09d485ccee81813331c8c6052d39344443b83da
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(CLI11)
endif()
target_link_libraries(rangequery_modules PRIVATE CLI11::CLI11)

add_executable(rangequery main.cpp)
target_link_libraries(rangequery PRIVATE rangequery_modules)

add_executable(rangequery_benchmark benchmark/main.cpp)
target_sources(rangequery_benchmark
  PRIVATE
    FILE_SET CXX_MODULES FILES
      benchmark/config.cppm
      benchmark/utils.cppm
)
target_link_libraries(rangequery_benchmark PRIVATE CLI11::CLI11)
find_package(benchmark 1.9.4 EXACT CONFIG QUIET)
if(NOT benchmark_FOUND)
  include(FetchContent)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(benchmark
    URL https://codeload.github.com/google/benchmark/tar.gz/refs/tags/v1.9.4
    URL_HASH SHA256=b334658edd35efcf06a99d9be21e4e93e092bd5f95074c1673d5c8705d95c104
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(benchmark)
  # Clang 23 diagnoses Google Benchmark 1.9.4's __COUNTER__ probe as a
  # C2y extension; its own -pedantic-errors otherwise stops the build.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 23)
    target_compile_options(benchmark PRIVATE -Wno-c2y-extensions)
    target_compile_options(benchmark_main PRIVATE -Wno-c2y-extensions)
  endif()
endif()

target_link_libraries(rangequery_benchmark PRIVATE rangequery_modules benchmark::benchmark)
target_compile_definitions(rangequery_benchmark PRIVATE
  RQ_BENCHMARK_VERSION="1.9.4"
  RQ_COMPILER="${CMAKE_CXX_COMPILER}"
  RQ_COMPILER_VERSION="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
  RQ_BUILD_CONFIG="$<IF:$<BOOL:$<CONFIG>>,$<CONFIG>,unspecified>"
)
