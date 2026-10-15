cmake_minimum_required(VERSION 3.28)

project(RangeQueryResearch LANGUAGES CXX)

option(RANGEQUERY_BUILD_TESTS "Собирать набор тестов GoogleTest" ON)
option(RANGEQUERY_ENABLE_SANITIZERS "Включить AddressSanitizer и UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if(RANGEQUERY_ENABLE_SANITIZERS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  else()
    message(FATAL_ERROR "Sanitizers are supported only with GCC or Clang")
  endif()
endif()

add_subdirectory(src)

if(RANGEQUERY_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
