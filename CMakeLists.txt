
# Copyright (c) 2010 Daniel Pfeifer <daniel@pfeifer-mail.de>
#               2010-2013 Stefan Eilemann <eile@eyescale.ch>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Collage)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

include(revision)

set(RELEASE_VERSION 1) # OFF or 'Mm0' ABI version
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_PATCH "1")
set(LAST_RELEASE 1.0.1)

include(Common)
include(GitTargets)
include(UpdateFile)

include(FindPackages)
set(COLLAGE_DEPENDENT_LIBRARIES Lunchbox)

set(FEATURES)
include_directories(BEFORE ${CMAKE_SOURCE_DIR})

if(LUNCHBOX_USE_OPENMP)
  find_package(OpenMP REQUIRED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()
if(LUNCHBOX_USE_DNSSD)
  set(FEATURES "${FEATURES} zeroconf")
endif()

if(NOT MSVC)
  set(PTHREAD_LIBRARIES pthread)
endif()

if(OFED_FOUND)
  set(FEATURES "${FEATURES} RDMA")
endif()
if(UDT_FOUND)
  if(NOT UDT_HAS_RCVDATA)
    message(STATUS "Disable old UDT version, missing UDT_RCVDATA")
    set(UDT_FOUND)
  else()
    set(FEATURES "${FEATURES} UDT")
  endif()
endif()

if(APPLE)
  add_definitions(-DDarwin)
endif(APPLE)

add_definitions(-DCO_BUILD_DIR="${CMAKE_BINARY_DIR}/")

add_subdirectory(tools)
add_subdirectory(tests)

add_subdirectory(co)
add_subdirectory(doc)

if(MSVC)
  message(STATUS "Configured with${FEATURES}")
else()
  message(STATUS "Configured with ${CMAKE_BUILD_TYPE}${FEATURES}")
endif()

include(CPackConfig)
include(DoxygenRule)

set(CTEST_CUSTOM_WARNING_EXCEPTION ${CTEST_CUSTOM_WARNING_EXCEPTION}
  # http://public.kitware.com/Bug/view.php?id=10179
  "ld: warning: duplicate dylib")
include(CTest)

include(PackageConfig)
