add_library(rangequery_modules)

target_sources(rangequery_modules
  PUBLIC
    FILE_SET CXX_MODULES FILES
      config/config.cppm
      structures/contracts.cppm
      structures/ordered_vector/ordered_vector.cppm
      structures/std_set/std_set.cppm
      structures/avl/avl.cppm
      structures/treap/treap.cppm
  PRIVATE
      config/config.cpp
      structures/ordered_vector/ordered_vector.cpp
      structures/std_set/std_set.cpp
      structures/avl/avl.cpp
      structures/treap/treap.cpp
)

target_compile_features(rangequery_modules PUBLIC cxx_std_23)

add_executable(rangequery main.cpp)
target_link_libraries(rangequery PRIVATE rangequery_modules)
