cmake_minimum_required(VERSION 3.28)

project(RangeQueryResearch LANGUAGES CXX)

option(RANGEQUERY_BUILD_TESTS "Собирать набор тестов GoogleTest" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_subdirectory(src)

if(RANGEQUERY_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
