
# Copyright (c) 2010-2016 Daniel Pfeifer <daniel@pfeifer-mail.de>
#                         Stefan Eilemann <eile@eyescale.ch>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Collage)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
  ${CMAKE_SOURCE_DIR}/CMake/common)

include(GitExternal)

option(COLLAGE_BUILD_V2_API
  "Enable for pure 2.0 API (breaks compatibility with 1.x API)" OFF)

set(VERSION_MAJOR "1")
set(VERSION_MINOR "4")
set(VERSION_PATCH "0")
set(VERSION_ABI 4)

set(COLLAGE_INCLUDE_NAME co)
include(Common)

list(APPEND CPPCHECK_EXTRA_ARGS -DCO_API=)
set(DPUT_HOST "ppa:eilemann/equalizer-dev")

set(COMMON_PROJECT_DOMAIN ch.eyescale)
set(COLLAGE_DESCRIPTION "High-performance C++ library for developing object-oriented distributed applications.")
set(COLLAGE_MAINTAINER "Stefan Eilemann <eile@eyescale.ch>")
set(COLLAGE_LICENSE LGPL)
set(COLLAGE_DEPENDENT_LIBRARIES Boost Lunchbox Pression)

common_package(Boost REQUIRED COMPONENTS system regex date_time serialization
                                         program_options thread)
common_package(Lunchbox REQUIRED)
common_package(OFED)
common_package(Pression REQUIRED)
common_package(Servus REQUIRED)
common_package(UDT)
common_package_post()

if(NOT MSVC)
  set(PTHREAD_LIBRARIES pthread)
endif()

if(APPLE)
  add_definitions(-DDarwin)
endif()

add_subdirectory(tools)
add_subdirectory(tests)

add_subdirectory(co)
add_subdirectory(doc)

include(CPackConfig)

set(DOXYGEN_MAINPAGE_MD README.md)
set(DOXYGEN_EXTRA_INPUT ${PROJECT_SOURCE_DIR}/README.md)
include(DoxygenRule)
