find_package(GTest CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_executable(rangequery_tests
  config_test.cpp
  structure_contract_test.cpp
)
target_link_libraries(rangequery_tests PRIVATE rangequery_modules GTest::gtest_main)
target_compile_features(rangequery_tests PRIVATE cxx_std_23)
add_dependencies(rangequery_tests rangequery rangequery_benchmark)

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
