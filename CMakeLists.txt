
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

set(LAST_RELEASE 1.1.1)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "1")
set(VERSION_PATCH "1")
set(VERSION_ABI 2)

set(GITTARGETS_RELEASE_BRANCH minor)
set(DPUT_HOST "ppa:eilemann/equalizer-dev")
set(PROJECT_INCLUDE_NAME co)

set(LUNCHBOX_DESCRIPTION "Multi-threaded C++ toolbox library for all application developers creating high-performance multi-threaded programs.")
set(COMMON_PROJECT_DOMAIN ch.eyescale)
set(COLLAGE_DESCRIPTION "High-performance C++ library for developing object-oriented distributed applications.")
set(COLLAGE_DEPENDENT_LIBRARIES Lunchbox)

include(Common)
include(FindPackages)

if(NOT MSVC)
  set(PTHREAD_LIBRARIES pthread)
endif()

if(APPLE)
  add_definitions(-DDarwin)
endif(APPLE)

add_subdirectory(tools)
add_subdirectory(tests)

add_subdirectory(co)
add_subdirectory(doc)

include(CPackConfig)

set(DOXYGEN_EXTRA_INPUT @CMAKE_BINARY_DIR@/doc/RelNotes.md)
include(DoxygenRule)
