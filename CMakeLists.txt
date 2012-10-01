
# Copyright (c) 2010 Daniel Pfeifer <daniel@pfeifer-mail.de>
#               2010-2012 Stefan Eilemann <eile@eyescale.ch>

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(Collage)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

include(revision)

set(RELEASE_VERSION OFF) # OFF or 'Mm0' ABI version
set(VERSION_MAJOR "0")
set(VERSION_MINOR "8")
set(VERSION_PATCH "0")
#set(LAST_RELEASE )

include(Common)
include(GitTargets)
include(FindPackages)
include(UpdateFile)

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

set(PTHREAD_LIBRARIES pthread)

if(OFED_FOUND)
  set(FEATURES "${FEATURES} RDMA")
endif()
if(UDT_FOUND)
  set(FEATURES "${FEATURES} UDT")
endif()

if(APPLE)
  add_definitions(-DDarwin)
endif(APPLE)

add_definitions(-DCO_BUILD_DIR="${CMAKE_BINARY_DIR}/")

add_subdirectory(tools)
add_subdirectory(tests)

add_subdirectory(co)
add_subdirectory(doc)

find_program(LINKS_EXECUTABLE links)
if(LINKS_EXECUTABLE)
  add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/RELNOTES.txt
    COMMAND ${LINKS_EXECUTABLE} -dump doc/RelNotes.dox > RELNOTES.txt
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/doc/RelNotes.dox)
  add_custom_target(RELNOTES.txt ALL DEPENDS ${CMAKE_SOURCE_DIR}/RELNOTES.txt)
endif()

find_package(PythonInterp)
if(PYTHON_EXECUTABLE)
  add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/README.rst
    COMMAND ${PYTHON_EXECUTABLE} CMake/html2rst.py doc/RelNotes.dox > README.rst
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/doc/RelNotes.dox)
  add_custom_target(README.rst ALL DEPENDS ${CMAKE_SOURCE_DIR}/README.rst)
endif()

install(FILES CMake/FindCollage.cmake
  DESTINATION ${CMAKE_MODULE_INSTALL_PATH} COMPONENT dev)
install(FILES CMake/FindLibraryPackage.cmake
  DESTINATION ${CMAKE_MODULE_INSTALL_PATH}/FindCollage COMPONENT dev)

if(MSVC)
  message(STATUS "Configured with${FEATURES}")
else()
  message(STATUS "Configured with ${CMAKE_BUILD_TYPE}${FEATURES}")
endif()

include(DoxygenRule)
include(CPackConfig)

set(CTEST_CUSTOM_WARNING_EXCEPTION ${CTEST_CUSTOM_WARNING_EXCEPTION}
  # http://public.kitware.com/Bug/view.php?id=10179
  "ld: warning: duplicate dylib")
include(CTest)
