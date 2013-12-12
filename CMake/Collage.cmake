

set(COLLAGE_PACKAGE_VERSION 1.0.0)
set(COLLAGE_DEPENDS OFED UDT REQUIRED Boost Lunchbox)
set(COLLAGE_DEB_DEPENDS librdmacm-dev libibverbs-dev librdmacm-dev libudt-dev
  libboost-date-time-dev libboost-regex-dev libboost-serialization-dev
  libboost-system-dev)
set(COLLAGE_BOOST_COMPONENTS "system regex date_time serialization")
set(COLLAGE_REPO_URL https://github.com/Eyescale/Collage.git)
set(COLLAGE_REPO_TAG 1.0)

if(CI_BUILD_COMMIT)
  set(COLLAGE_REPO_TAG ${CI_BUILD_COMMIT})
else()
  set(COLLAGE_REPO_TAG master)
endif()
set(COLLAGE_FORCE_BUILD ON)
set(COLLAGE_SOURCE ${CMAKE_SOURCE_DIR})
