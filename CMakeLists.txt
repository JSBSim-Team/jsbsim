cmake_minimum_required (VERSION 2.8.7)

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

option(SYSTEM_EXPAT "Set to ON to build JSBSim using the system libExpat" OFF)
if (SYSTEM_EXPAT)
  find_package(EXPAT)
endif()

################################################################################
# Build JSBSim libs and exec                                                   #
################################################################################

add_subdirectory(src)

################################################################################
# Build the automated test infrastructure (needs Python and Cython)            #
################################################################################

option(INSTALL_PYTHON_MODULE "Set to ON to install the Python module for JSBSim" OFF)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/tests/CMakeModules)
find_package(Cython)

if (CYTHON_FOUND)
  find_package(PythonLibs)
  if (PYTHONLIBS_FOUND)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
    enable_testing()
    add_subdirectory(tests)
  endif(PYTHONLIBS_FOUND)
endif(CYTHON_FOUND)

if (NOT (CYTHON_FOUND AND PYTHONLIBS_FOUND))
    message(WARNING "JSBSim Python module and test suite will not be built")
endif()

################################################################################
# Packaging                                                                    #
################################################################################

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
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
                               "TAGS"
                               "${CPACK_SOURCE_IGNORE_FILES}")
include (CPack)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

################################################################################
# Build the utilities                                                          #
################################################################################

add_subdirectory(utils)

