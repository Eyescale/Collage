
# Copyright (c) 2011-2013 Stefan Eilemann <eile@eyescale.ch>

configure_file(RelNotes.md.in ${CMAKE_CURRENT_BINARY_DIR}/RelNotes.md @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RelNotes.md ../AUTHORS ../CHANGES.txt
              ../LGPL ../LICENSE.txt
        DESTINATION share/Collage/doc COMPONENT doc)
