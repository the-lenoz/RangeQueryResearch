find_package(GTest CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_executable(rangequery_tests
  config_test.cpp
  factory_test.cpp
  structure_contract_test.cpp
  value_semantics_test.cpp
)
target_sources(rangequery_tests PRIVATE
  FILE_SET CXX_MODULES BASE_DIRS ${PROJECT_SOURCE_DIR}/src FILES
    ${PROJECT_SOURCE_DIR}/src/benchmark/config.cppm
)
target_link_libraries(rangequery_tests PRIVATE
  rangequery_modules CLI11::Module GTest::gtest_main GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(rangequery_tests DISCOVERY_MODE PRE_TEST)

add_test(
  NAME rangequery_cli_integration
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cli_integration_test.py
          --binary $<TARGET_FILE:rangequery>
)

add_test(
  NAME rangequery_benchmark_integration
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_integration_test.py
          --runner $<TARGET_FILE:rangequery_benchmark>
)
