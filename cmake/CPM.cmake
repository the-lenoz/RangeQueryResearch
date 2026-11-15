# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: Copyright (c) 2019-2023 Lars Melchior and contributors

set(CPM_DOWNLOAD_VERSION 0.42.0)
set(CPM_HASH_SUM "2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a")

if(CPM_SOURCE_CACHE)
  set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
  set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

# Expand relative path. This is important if the provided path contains a tilde (~)
get_filename_component(CPM_DOWNLOAD_LOCATION "${CPM_DOWNLOAD_LOCATION}" ABSOLUTE)

# Verify cached copies as well as new downloads before executing CMake code.
if(EXISTS "${CPM_DOWNLOAD_LOCATION}")
  file(SHA256 "${CPM_DOWNLOAD_LOCATION}" CPM_ACTUAL_HASH)
  if(NOT CPM_ACTUAL_HASH STREQUAL CPM_HASH_SUM)
    message(FATAL_ERROR "CPM ${CPM_DOWNLOAD_VERSION}: SHA-256 mismatch in ${CPM_DOWNLOAD_LOCATION}; remove the corrupt cache file and retry")
  endif()
else()
  get_filename_component(CPM_DOWNLOAD_DIRECTORY "${CPM_DOWNLOAD_LOCATION}" DIRECTORY)
  file(MAKE_DIRECTORY "${CPM_DOWNLOAD_DIRECTORY}")
  file(DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
    "${CPM_DOWNLOAD_LOCATION}.tmp"
    STATUS CPM_DOWNLOAD_STATUS
    TLS_VERIFY ON
  )
  list(GET CPM_DOWNLOAD_STATUS 0 CPM_DOWNLOAD_CODE)
  list(GET CPM_DOWNLOAD_STATUS 1 CPM_DOWNLOAD_MESSAGE)
  if(NOT CPM_DOWNLOAD_CODE EQUAL 0)
    file(REMOVE "${CPM_DOWNLOAD_LOCATION}.tmp")
    message(FATAL_ERROR "CPM ${CPM_DOWNLOAD_VERSION}: download failed: ${CPM_DOWNLOAD_MESSAGE}")
  endif()
  file(SHA256 "${CPM_DOWNLOAD_LOCATION}.tmp" CPM_ACTUAL_HASH)
  if(NOT CPM_ACTUAL_HASH STREQUAL CPM_HASH_SUM)
    file(REMOVE "${CPM_DOWNLOAD_LOCATION}.tmp")
    message(FATAL_ERROR "CPM ${CPM_DOWNLOAD_VERSION}: downloaded file has an unexpected SHA-256")
  endif()
  file(RENAME "${CPM_DOWNLOAD_LOCATION}.tmp" "${CPM_DOWNLOAD_LOCATION}")
endif()

include("${CPM_DOWNLOAD_LOCATION}")
