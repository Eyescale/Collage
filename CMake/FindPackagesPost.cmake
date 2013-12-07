
if(UDT_FOUND AND NOT UDT_HAS_RCVDATA)
  message(STATUS "Disable old UDT version, missing UDT_RCVDATA")
  set(UDT_FOUND)
endif()

if(LUNCHBOX_USE_DNSSD)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} zeroconf")
endif()
