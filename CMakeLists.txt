cmake_minimum_required (VERSION 2.8.0)

################################################################################
# Project description                                                          #
################################################################################
project(JSBSim)

set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "0")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(PROJECT_CONTACT_EMAIL jon@jsbsim.org)
set(PROJECT_CONTACT_VENDOR JSBSim)
set(LIBRARY_VERSION ${PROJECT_VERSION})
set(LIBRARY_SOVERSION ${PROJECT_VERSION_MAJOR})

option(SYSTEM_EXPAT     "Set to ON to build SimGear using the system libExpat" OFF)

################################################################################
# Build JSBSim libs and exec                                                   #
################################################################################

add_subdirectory(src)

################################################################################
# Packaging                                                                    #
################################################################################

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CPACK_RESOURCE_FILE_README}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JSBSim, an open source, platform-independent, flight dynamics & control software library in C++.")
set(CPACK_PACKAGE_VENDOR ${PROJECT_CONTACT_VENDOR})
set(CPACK_PACKAGE_CONTACT "${PROJECT_CONTACT_EMAIL}")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # Extract automatically the package dependencies
set(CPACK_SET_DESTDIR TRUE)
set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_CURRENT_BINARY_DIR}/"
                               "/admin/"
                               "/autom4te.cache"
                               "/Makefile$"
                               "/Makefile.in$"
                               "depcomp$"
                               "install-sh$"
                               "libtool$"
                               "ltmain.sh"
                               "missing"
                               "/.*\\\\.csv$"
                               "/.*\\\\.o$"
                               "/.*\\\\.a$"
                               "/.*\\\\.m4$"
                               "/.cvsignore$"
                               "config.guess"
                               "config.log"
                               "config.status"
                               "config.sub"
                               "configure$"
                               "/JSBSim$"
                               "/.deps/"
                               "/.libs/"
                               "/CVS/"
                               "${CPACK_SOURCE_IGNORE_FILES}")
include (CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

