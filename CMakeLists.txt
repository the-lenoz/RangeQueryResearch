cmake_minimum_required(VERSION 3.28)

project(RangeQueryResearch LANGUAGES CXX)

option(RANGEQUERY_BUILD_TESTS "Собирать набор тестов GoogleTest" ON)
option(RANGEQUERY_ENABLE_SANITIZERS "Включить AddressSanitizer и UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_program(RANGEQUERY_CLANG_FORMAT NAMES clang-format-18 clang-format)
file(GLOB_RECURSE RANGEQUERY_FORMAT_SOURCES
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cppm"
)

if(RANGEQUERY_CLANG_FORMAT)
  add_custom_target(format
    COMMAND ${RANGEQUERY_CLANG_FORMAT} --style=file -i ${RANGEQUERY_FORMAT_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Formatting C++ sources"
    VERBATIM
  )

  add_custom_target(format-check
    COMMAND ${RANGEQUERY_CLANG_FORMAT} --style=file --dry-run --Werror
            ${RANGEQUERY_FORMAT_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking C++ source formatting"
    VERBATIM
  )
else()
  message(STATUS "clang-format was not found; format targets are unavailable")
endif()

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
