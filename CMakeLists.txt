
# Copyright (c) 2010-2017 Daniel Pfeifer <daniel@pfeifer-mail.de>
#                         Stefan Eilemann <eile@eyescale.ch>

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(Collage VERSION 1.7.0)
set(Collage_VERSION_ABI 7)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
                              ${CMAKE_SOURCE_DIR}/CMake/common)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/CMake/common/Common.cmake)
  message(FATAL_ERROR "CMake/common missing, run: git submodule update --init")
endif()

option(COLLAGE_BUILD_V2_API
  "Enable for pure 2.0 API (breaks compatibility with 1.x API)" OFF)

set(COLLAGE_INCLUDE_NAME co)
set(COLLAGE_DEB_DEPENDS librdmacm-dev libibverbs-dev librdmacm-dev libudt-dev
  libboost-date-time-dev libboost-regex-dev libboost-serialization-dev
  libboost-system-dev libboost-thread-dev libboost-program-options-dev)
set(COLLAGE_PORT_DEPENDS boost)

include(Common)

list(APPEND CPPCHECK_EXTRA_ARGS -DCO_API=)
set(DPUT_HOST "ppa:eilemann/equalizer-dev")

set(COMMON_PROJECT_DOMAIN ch.eyescale)
set(COLLAGE_DESCRIPTION "High-performance C++ library for developing object-oriented distributed applications.")
set(COLLAGE_MAINTAINER "Stefan Eilemann <eile@eyescale.ch>")
set(COLLAGE_LICENSE LGPL)
set(COLLAGE_DEPENDENT_LIBRARIES Boost Lunchbox Pression)

common_find_package(Boost REQUIRED COMPONENTS system regex date_time
  serialization program_options thread)
common_find_package(Lunchbox REQUIRED)
common_find_package(OFED)
common_find_package(OpenMP)
common_find_package(Pression REQUIRED)
common_find_package(Servus REQUIRED)
common_find_package(UDT)

if(UDT_FOUND AND NOT UDT_HAS_RCVDATA)
  message(STATUS "Disable old UDT version, missing UDT_RCVDATA")
  set(UDT_FOUND)
endif()

if(COLLAGE_AGGRESSIVE_CACHING)
  list(APPEND COMMON_FIND_PACKAGE_DEFINES CO_AGGRESSIVE_CACHING)
endif()

if(COLLAGE_BUILD_V2_API)
  list(APPEND COMMON_FIND_PACKAGE_DEFINES COLLAGE_V2_API)
else()
  list(APPEND COMMON_FIND_PACKAGE_DEFINES COLLAGE_V1_API)
endif()

common_find_package_post()

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
