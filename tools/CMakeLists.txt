# Copyright (c) 2010 Daniel Pfeifer <daniel@pfeifer-mail.de>

include(ParseArguments)
include(PurpleInstallPDB)
include(PurpleExpandLibraries)

macro(CO_ADD_TOOL NAME)
  parse_arguments(THIS "SOURCES;HEADERS;LINK_LIBRARIES" "" ${ARGN})
  set(THIS_TARGET ${NAME})
  source_group(\\ FILES ${THIS_SOURCES} ${THIS_HEADERS})
  add_executable(${THIS_TARGET} ${THIS_SOURCES} ${THIS_HEADERS})
  purple_expand_libraries(LINK_LIBRARIES ${THIS_LINK_LIBRARIES})
  target_link_libraries(${THIS_TARGET} ${LINK_LIBRARIES})
  set_target_properties(${THIS_TARGET} PROPERTIES OUTPUT_NAME ${NAME} FOLDER "Tools")
  install(TARGETS ${THIS_TARGET} DESTINATION bin COMPONENT apps)
  purple_install_pdb(${THIS_TARGET} DESTINATION bin COMPONENT apps)
endmacro(CO_ADD_TOOL NAME)

co_add_tool(coNetperf
  SOURCES netperf/netperf.cpp
  LINK_LIBRARIES shared Collage
  )
