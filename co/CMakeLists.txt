
# Copyright (c) 2010 Daniel Pfeifer <daniel@pfeifer-mail.de>
#               2010-2012 Stefan Eilemann <eile@eyescale.ch>
#               2010 Cedric Stalder <cedric.stalder@gmail.ch>

include(configure.cmake)
include(files.cmake)

option(COLLAGE_AGGRESSIVE_CACHING "Disable to reduce memory consumption" ON)
mark_as_advanced(COLLAGE_AGGRESSIVE_CACHING)

set(CO_ADD_LINKLIB ${PTHREAD_LIBRARIES} ${LUNCHBOX_LIBRARIES})

if(WIN32)
  list(APPEND CO_ADD_LINKLIB ws2_32 mswsock)
endif(WIN32)
if(LINUX)
  list(APPEND CO_ADD_LINKLIB dl rt)
endif()

if(Boost_FOUND)
  list(APPEND CO_SOURCES dataIStreamArchive.cpp dataOStreamArchive.cpp
                         rspConnection.cpp)
  list(APPEND CO_ADD_LINKLIB ${Boost_SERIALIZATION_LIBRARY}
                             ${Boost_SYSTEM_LIBRARY})
  if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND CO_ADD_LINKLIB ${Boost_DATE_TIME_LIBRARY})
  endif()
  add_definitions(-DBOOST_ARCHIVE_SOURCE)
endif(Boost_FOUND)

if(OFED_FOUND)
  include_directories(SYSTEM ${OFED_INCLUDE_DIRS})
  list(APPEND CO_HEADERS rdmaConnection.h)
  list(APPEND CO_SOURCES rdmaConnection.cpp)
  list(APPEND CO_ADD_LINKLIB ${OFED_LIBRARIES})
  if(WIN32)
    list(APPEND CO_ADD_LINKLIB DelayImp.lib)
  endif(WIN32)
endif()

if(UDT_FOUND)
  include_directories(SYSTEM ${UDT_INCLUDE_DIRS})
  list(APPEND CO_HEADERS udtConnection.h)
  list(APPEND CO_SOURCES udtConnection.cpp)
  list(APPEND CO_ADD_LINKLIB ${UDT_LIBRARIES})
endif()

source_group(\\ FILES CMakeLists.txt)
source_group(collage FILES ${CO_PUBLIC_HEADERS} ${CO_HEADERS} ${CO_SOURCES} )

add_library(Collage SHARED ${CO_PUBLIC_HEADERS} ${CO_HEADERS} ${CO_SOURCES})
target_link_libraries(Collage ${CO_ADD_LINKLIB})
set_target_properties(Collage
  PROPERTIES VERSION ${VERSION} SOVERSION ${VERSION_ABI})

install(FILES ${CO_PUBLIC_HEADERS} DESTINATION include/co COMPONENT dev)
install(TARGETS Collage
        ARCHIVE DESTINATION lib COMPONENT dev
        RUNTIME DESTINATION bin COMPONENT lib
        LIBRARY DESTINATION lib COMPONENT lib)

if(OFED_FOUND AND WIN32)
  set_target_properties(Collage PROPERTIES LINK_FLAGS "/DELAYLOAD:librdmacm.dll /DELAYLOAD:libibverbs.dll")
endif()

install(FILES ${OPTIONS_CMAKE} DESTINATION ${CMAKE_MODULE_INSTALL_PATH}
  COMPONENT dev)
