# Copyright (c) 2015-2016 Stefan.Eilemann@epfl.ch

macro(CO_ADD_TOOL Name)
  string(TOUPPER ${Name} NAME)
  set(${NAME}_SOURCES ${ARGN})
  set(${NAME}_LINK_LIBRARIES Collage ${Boost_PROGRAM_OPTIONS_LIBRARY})
  common_application(${Name})
endmacro()

# Avoid link errors with boost on windows
add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK)

co_add_tool(coNetperf netperf.cpp)
co_add_tool(coNodeperf nodeperf.cpp)
co_add_tool(coObjectperf objectperf.cpp)
