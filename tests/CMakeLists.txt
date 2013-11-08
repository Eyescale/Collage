# Copyright (c) 2010-2013, Stefan Eilemann <eile@eyescale.ch>
#
# Change this number when adding tests to force a CMake run: 4

include(InstallFiles)

include_directories(${CMAKE_SOURCE_DIR}) # some tests need private headers
set(TEST_LIBRARIES Collage)
include(CommonCTest)

install_files(share/Collage/tests FILES ${TEST_FILES} COMPONENT examples)
