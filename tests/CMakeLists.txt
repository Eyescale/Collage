# Copyright (c) 2010 Daniel Pfeifer
#               2010-2013, Stefan Eilemann <eile@eyescale.ch>
#
# Change this number when adding tests to force a CMake run: 2

include(InstallFiles)

if(NOT WIN32) # tests want to be with DLLs on Windows - no rpath
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR} # some tests need private headers
  )

file(GLOB_RECURSE TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
list(SORT TEST_FILES)
install_files(share/Collage/tests FILES ${TEST_FILES} COMPONENT examples)

if($ENV{TRAVIS})
  list(REMOVE_ITEM TEST_FILES "zeroconf.cpp")
endif()

set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
  string(REGEX REPLACE ".cpp" "" NAME ${FILE})
  string(REGEX REPLACE "[./]" "_" NAME ${NAME})
  source_group(\\ FILES ${FILE})
  add_executable(${NAME} ${FILE})
  set_target_properties(${NAME} PROPERTIES FOLDER "Tests")

  target_link_libraries(${NAME} Collage)

  get_target_property(EXECUTABLE ${NAME} LOCATION)
  STRING(REGEX REPLACE "\\$\\(.*\\)" "\${CTEST_CONFIGURATION_TYPE}"
         EXECUTABLE "${EXECUTABLE}")

  add_test(${NAME} ${EXECUTABLE})
  if(APPLE)
    set_property(TEST ${NAME} PROPERTY ENVIRONMENT
      "DYLD_LIBRARY_PATH=${LUNCHBOX_LIBRARY_DIRS}:$ENV{DYLD_LIBRARY_PATH}"
      )
  endif()
  list(APPEND ALL_TESTS ${NAME})
endforeach(FILE ${TEST_FILES})

# workaround bug that tests won't get built
add_custom_target(tests COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS ${ALL_TESTS})
set_target_properties(tests PROPERTIES FOLDER "Tests")
