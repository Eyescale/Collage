# Copyright (c) 2010 Daniel Pfeifer <daniel@pfeifer-mail.de>
#               2013 Stefan.Eilemann@epfl.ch

if(NOT tclap_FOUND)
  message(STATUS "Missing TCLAP command line parser, tools not available")
  return()
endif()

include(CMakeParseArguments)
include(PurpleInstallPDB)

macro(CO_ADD_TOOL NAME)
  cmake_parse_arguments(THIS "" "" "SOURCES" ${ARGN})
  set(THIS_TARGET ${NAME})
  source_group(\\ FILES ${THIS_SOURCES})
  add_executable(${THIS_TARGET} ${THIS_SOURCES})
  target_link_libraries(${THIS_TARGET} Collage)
  set_target_properties(${THIS_TARGET} PROPERTIES OUTPUT_NAME ${NAME} FOLDER "Tools")
  install(TARGETS ${THIS_TARGET} DESTINATION bin COMPONENT apps)
  purple_install_pdb(${THIS_TARGET} DESTINATION bin COMPONENT apps)
endmacro(CO_ADD_TOOL NAME)

co_add_tool(coNetperf SOURCES perf/netperf.cpp)
co_add_tool(coNodeperf SOURCES perf/nodeperf.cpp)
co_add_tool(coObjectperf SOURCES perf/objectperf.cpp)
