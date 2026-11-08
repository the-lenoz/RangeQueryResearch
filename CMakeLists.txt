cmake_minimum_required(VERSION 3.28)

project(RangeQueryResearch LANGUAGES CXX)

option(RANGEQUERY_BUILD_TESTS "Build GoogleTest" ON)
option(RANGEQUERY_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_program(RANGEQUERY_CLANG_FORMAT NAMES clang-format-18 clang-format)
if(RANGEQUERY_CLANG_FORMAT)
  file(GLOB_RECURSE RANGEQUERY_FORMAT_SOURCES
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cppm"
  )
  list(FILTER RANGEQUERY_FORMAT_SOURCES EXCLUDE REGEX "/\\.#[^/]*$")

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

add_subdirectory(src)

if(RANGEQUERY_ENABLE_SANITIZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Sanitizers are supported only with GCC or Clang")
  endif()

  target_compile_options(rangequery_modules PUBLIC
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-fsanitize=address,undefined>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-fno-omit-frame-pointer>
  )
  target_link_options(rangequery_modules PUBLIC
    $<$<LINK_LANG_AND_ID:CXX,GNU,Clang>:-fsanitize=address,undefined>
  )
endif()

if(RANGEQUERY_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(tests)
  endif()
endif()
