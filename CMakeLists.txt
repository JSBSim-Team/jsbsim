cmake_minimum_required (VERSION 2.8.7)

################################################################################
# Project description                                                          #
################################################################################
project(JSBSim)

set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "0")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_CONTACT_EMAIL "jsbsim-devel@lists.sourceforge.net")
set(PROJECT_CONTACT_VENDOR "The JSBSim team")

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
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

function(get_tail INPUT_STRING OUTPUT_STRING)
  string(REPLACE "\t" " " TEMP_LIST ${INPUT_STRING})
  separate_arguments(TEMP_LIST)
  list(GET TEMP_LIST -1 TEMP_TAIL)
  string(STRIP ${TEMP_TAIL} TAIL)
  set(${OUTPUT_STRING} ${TAIL} PARENT_SCOPE)
endfunction()

# Extract the host architecture for the package name
if (UNIX)
  execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(LINUX_DISTRIB_NAME "FreeBSD")
  else()
    execute_process(COMMAND lsb_release -i OUTPUT_VARIABLE DISTRIB_NAME)
    execute_process(COMMAND lsb_release -r OUTPUT_VARIABLE DISTRIB_RELEASE)
    get_tail(${DISTRIB_NAME} LINUX_DISTRIB_NAME)
    get_tail(${DISTRIB_RELEASE} LINUX_DISTRIB_RELEASE)
    string(TOLOWER ${LINUX_DISTRIB_NAME} TEMP_NAME)
    if (TEMP_NAME STREQUAL "fedora")
      set(LINUX_DISTRIB_NAME ".fc")
    elseif(TEMP_NAME STREQUAL "ubuntu")
      if(LINUX_DISTRIB_RELEASE STREQUAL "14.04")
        set(LINUX_DISTRIB_NAME "~trusty")
        set(LINUX_DISTRIB_RELEASE)
      endif()
      if(ARCH STREQUAL "x86_64")
        set(ARCH "amd64")
      endif()
    else()
      set(LINUX_DISTRIB_NAME ".${LINUX_DISTRIB_NAME}")
    endif()
  endif()
  if (DEFINED ENV{TRAVIS})
      set(LINUX_DISTRIB_NAME "-$ENV{TRAVIS_BUILD_NUMBER}${LINUX_DISTRIB_NAME}")
  endif()
  set(HOST_ARCHITECTURE "${LINUX_DISTRIB_NAME}${LINUX_DISTRIB_RELEASE}.${ARCH}")
endif()

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(EXTENSION_NAME "_${CPACK_PACKAGE_VERSION}${HOST_ARCHITECTURE}")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CPACK_RESOURCE_FILE_README}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "JSBSim, an open source, platform-independent, flight dynamics & control software library in C++.")
set(CPACK_PACKAGE_VENDOR ${PROJECT_CONTACT_VENDOR})
set(CPACK_PACKAGE_CONTACT "${PROJECT_CONTACT_EMAIL}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}${EXTENSION_NAME}")
string(TOLOWER ${CMAKE_PROJECT_NAME} CPACK_PACKAGE_NAME)

################################################################################
# Debian/Ubuntu packaging                                                                    #
################################################################################

set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # Extract automatically the package dependencies
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_DEBIAN_RUNTIME_FILE_NAME "${CMAKE_PROJECT_NAME}${EXTENSION_NAME}.deb")
set(CPACK_DEBIAN_DEVEL_FILE_NAME "${CMAKE_PROJECT_NAME}-devel${EXTENSION_NAME}.deb")
set(CPACK_DEBIAN_PYMODULES_PACKAGE_NAME "python${PYTHON_VERSION_MAJOR}-${CPACK_PACKAGE_NAME}")
set(CPACK_DEBIAN_PYMODULES_FILE_NAME "python${PYTHON_VERSION_MAJOR}-${CMAKE_PROJECT_NAME}${EXTENSION_NAME}.deb")
if(BUILD_SHARED_LIBS)
  set(CPACK_DEBIAN_DEVEL_PACKAGE_DEPENDS "${CPACK_DEBIAN_RUNTIME_PACKAGE_NAME} (>=${CPACK_PACKAGE_VERSION})")
  set(CPACK_DEBIAN_PYMODULES_PACKAGE_DEPENDS "${CPACK_DEBIAN_RUNTIME_PACKAGE_NAME} (>=${CPACK_PACKAGE_VERSION})")
endif()

################################################################################
# Red Hat packaging                                                                    #
################################################################################

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_RUNTIME_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_RPM_RUNTIME_FILE_NAME "${CMAKE_PROJECT_NAME}${EXTENSION_NAME}.rpm")
set(CPACK_RPM_DEVEL_FILE_NAME "${CMAKE_PROJECT_NAME}-devel${EXTENSION_NAME}.rpm")
set(CPACK_RPM_PYMODULES_PACKAGE_NAME "python${PYTHON_VERSION_MAJOR}-${CPACK_PACKAGE_NAME}")
set(CPACK_RPM_PYMODULES_FILE_NAME "python${PYTHON_VERSION_MAJOR}-${CMAKE_PROJECT_NAME}${EXTENSION_NAME}.rpm")
set(CPACK_RPM_PACKAGE_LICENSE "LGPL-2.1")
if(BUILD_SHARED_LIBS)
  set(CPACK_RPM_DEVEL_PACKAGE_REQUIRES "${CPACK_RPM_RUNTIME_PACKAGE_NAME} >= ${CPACK_PACKAGE_VERSION}")
  set(CPACK_RPM_PYMODULES_PACKAGE_REQUIRES "${CPACK_RPM_RUNTIME_PACKAGE_NAME} >= ${CPACK_PACKAGE_VERSION}")
endif()

################################################################################
# Source packaging                                                                    #
################################################################################

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
