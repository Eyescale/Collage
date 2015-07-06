# Copyright (c) 2010 Daniel Pfeifer <daniel@pfeifer-mail.de>
#               2010-2012 Stefan Eilemann <eile@eyescale.ch>

#info: http://www.itk.org/Wiki/CMake:Component_Install_With_CPack

set(CPACK_PACKAGE_VENDOR "www.eyescale.ch")
set(CPACK_PACKAGE_CONTACT "Stefan Eilemann <eile@eyescale.ch>")
set(CPACK_PACKAGE_DESCRIPTION_FILE ${PROJECT_SOURCE_DIR}/doc/Changelog.md)
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
set(CPACK_PACKAGE_LICENSE "LGPL")

set(CPACK_DEBIAN_BUILD_DEPENDS
  ${COLLAGE_BUILD_DEBS} ${LUNCHBOX_DEB_DEV_DEPENDENCY})
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6, libboost-system-dev, libboost-date-time-dev, libboost-regex-dev, libboost-serialization-dev, libboost-thread-dev, librdmacm-dev, libibverbs-dev, librdmacm-dev, libudt-dev, ${LUNCHBOX_DEB_DEV_DEPENDENCY}")

set(CPACK_MACPORTS_CATEGORY devel)
set(CPACK_MACPORTS_DEPENDS boost Lunchbox)

include(CommonCPack)
include(OSSCPack)
