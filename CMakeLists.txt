
# Copyright (c) 2010 Daniel Pfeifer <daniel@pfeifer-mail.de>
#               2010-2014 Stefan Eilemann <eile@eyescale.ch>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Collage)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake
  ${CMAKE_SOURCE_DIR}/CMake/common)

include(GitExternal)
include(Buildyard)

if(BUILDYARD_STOP)
  return()
endif()

option(COLLAGE_BUILD_V2_API
  "Enable for pure 2.0 API (breaks compatibility with 1.x API)" OFF)

set(RELEASE_VERSION OFF) # OFF or 'Mm0' ABI version
set(VERSION_MAJOR "1")
set(VERSION_MINOR "1")
set(VERSION_PATCH "0")
set(LAST_RELEASE 1.0.0)

set(PROJECT_INCLUDE_NAME co)
set(COMMON_PROJECT_DOMAIN ch.eyescale)
set(COLLAGE_DESCRIPTION "High-performance C++ library for developing object-oriented distributed applications.")
set(COLLAGE_DEPENDENT_LIBRARIES Lunchbox)

include(Common)
include(FindPackages)

list(APPEND CPPCHECK_EXTRA_ARGS -DCO_API=)

if(LUNCHBOX_USE_OPENMP)
  find_package(OpenMP REQUIRED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

if(NOT MSVC)
  set(PTHREAD_LIBRARIES pthread)
endif()

if(APPLE)
  add_definitions(-DDarwin)
endif(APPLE)

add_definitions(-DCO_BUILD_DIR="${CMAKE_BINARY_DIR}/")

add_subdirectory(tools)
add_subdirectory(tests)

add_subdirectory(co)
add_subdirectory(doc)

include(CPackConfig)
include(DoxygenRule)
