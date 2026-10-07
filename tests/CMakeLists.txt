find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
  message(WARNING "GTest was not found; tests are skipped. Install GTest or set CMAKE_PREFIX_PATH.")
  return()
endif()

add_executable(rangequery_tests structure_contract_test.cpp)
target_link_libraries(rangequery_tests PRIVATE rangequery_modules GTest::gtest_main)
target_compile_features(rangequery_tests PRIVATE cxx_std_23)

include(GoogleTest)
gtest_discover_tests(rangequery_tests)
