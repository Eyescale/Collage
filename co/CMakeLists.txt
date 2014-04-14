
# Copyright (c) 2010 Daniel Pfeifer <daniel@pfeifer-mail.de>
#               2010-2013 Stefan Eilemann <eile@eyescale.ch>
#               2010 Cedric Stalder <cedric.stalder@gmail.ch>

include(files.cmake)

option(COLLAGE_AGGRESSIVE_CACHING "Disable to reduce memory consumption" ON)
mark_as_advanced(COLLAGE_AGGRESSIVE_CACHING)

list(APPEND COLLAGE_LINK_LIBRARIES ${PTHREAD_LIBRARIES} ${LUNCHBOX_LIBRARIES}
  ${Boost_SERIALIZATION_LIBRARY} ${Boost_SYSTEM_LIBRARY})

if(NOT Boost_USE_STATIC_LIBS)
  list(APPEND COLLAGE_LINK_LIBRARIES ${Boost_DATE_TIME_LIBRARY})
endif()
add_definitions(-DBOOST_ARCHIVE_SOURCE)

if(WIN32)
  list(APPEND COLLAGE_LINK_LIBRARIES ws2_32 mswsock)
endif(WIN32)
if(LINUX)
  list(APPEND COLLAGE_LINK_LIBRARIES dl rt)
endif()

if(OFED_FOUND)
  list(APPEND COLLAGE_LINK_LIBRARIES ${OFED_LIBRARIES})
  if(WIN32)
    list(APPEND COLLAGE_LINK_LIBRARIES DelayImp.lib)
  endif(WIN32)
endif()

if(UDT_FOUND)
  list(APPEND COLLAGE_LINK_LIBRARIES ${UDT_LIBRARIES})
endif()

if (MPI_FOUND)
  list(APPEND CMAKE_C_FLAGS ${MPI_C_COMPILE_FLAGS})
  list(APPEND CMAKE_CXX_FLAGS ${MPI_CXX_COMPILE_FLAGS})
  list(APPEND COLLAGE_LINK_LIBRARIES ${MPI_LIBRARIES})
endif()

common_library(Collage)

source_group(\\ FILES CMakeLists.txt)
source_group(collage FILES ${CO_PUBLIC_HEADERS} ${CO_HEADERS} ${CO_SOURCES} )

if (MSVC)
  if(OFED_FOUND)
    set_target_properties(Collage PROPERTIES
      LINK_FLAGS "/DELAYLOAD:librdmacm.dll /DELAYLOAD:libibverbs.dll")
  endif()

  install(FILES ${CMAKE_BINARY_DIR}/bin/Debug/Collage.pdb
    DESTINATION bin COMPONENT lib CONFIGURATIONS Debug)
  install(FILES ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/Collage.pdb
    DESTINATION bin COMPONENT lib CONFIGURATIONS RelWithDebInfo)
endif()
