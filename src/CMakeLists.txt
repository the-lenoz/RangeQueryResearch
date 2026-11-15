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

target_link_libraries(rangequery_modules PRIVATE CLI11::Module)

add_executable(rangequery main.cpp)
target_link_libraries(rangequery PRIVATE rangequery_modules)

add_executable(rangequery_benchmark benchmark/main.cpp)
target_sources(rangequery_benchmark
  PRIVATE
    FILE_SET CXX_MODULES FILES
      benchmark/config.cppm
      benchmark/utils.cppm
)
target_link_libraries(rangequery_benchmark PRIVATE CLI11::Module)
target_link_libraries(rangequery_benchmark PRIVATE rangequery_modules benchmark::benchmark)
target_compile_definitions(rangequery_benchmark PRIVATE
  RQ_BENCHMARK_VERSION="${RANGEQUERY_BENCHMARK_VERSION}"
  RQ_COMPILER="${CMAKE_CXX_COMPILER}"
  RQ_COMPILER_VERSION="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
  RQ_BUILD_CONFIG="$<IF:$<BOOL:$<CONFIG>>,$<CONFIG>,unspecified>"
)
