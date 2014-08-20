# Copyright (c) 2010-2013, Stefan Eilemann <eile@eyescale.ch>
#
# Change this number when adding tests to force a CMake run: 7

add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK)
set(TEST_LIBRARIES Collage)
include(CommonCTest)

install(FILES ${TEST_FILES} DESTINATION share/Collage/tests COMPONENT examples)
