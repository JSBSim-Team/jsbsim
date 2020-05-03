cmake_minimum_required (VERSION 2.8.8)

################################################################################
# Project description                                                          #
################################################################################
project(JSBSim)

set(PROJECT_VERSION_MAJOR "1")
set(PROJECT_VERSION_MINOR "1")
set(PROJECT_VERSION_PATCH "0.dev1")
set(PROJECT_CONTACT_EMAIL "jsbsim-devel@lists.sourceforge.net")
set(PROJECT_CONTACT_VENDOR "The JSBSim team")

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(LIBRARY_VERSION ${PROJECT_VERSION})
set(LIBRARY_SOVERSION ${PROJECT_VERSION_MAJOR})

option(SYSTEM_EXPAT "Set to ON to build JSBSim using the system libExpat" OFF)
if (SYSTEM_EXPAT)
  find_package(EXPAT)
endif()

function(get_tail INPUT_STRING OUTPUT_STRING SEPARATOR)
  string(REPLACE ${SEPARATOR} " " TEMP_LIST ${INPUT_STRING})
  separate_arguments(TEMP_LIST)
  list(GET TEMP_LIST -1 TEMP_TAIL)
  string(STRIP ${TEMP_TAIL} TAIL)
  set(${OUTPUT_STRING} ${TAIL} PARENT_SCOPE)
endfunction()

################################################################################
# Build JSBSim libs and exec                                                   #
################################################################################

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(src)

################################################################################
# Documentation                                                                #
################################################################################

find_package(Doxygen)
if(DOXYGEN_FOUND)
  execute_process(COMMAND date "+%d %b %Y" OUTPUT_VARIABLE TODAY OUTPUT_STRIP_TRAILING_WHITESPACE)
  configure_file(${CMAKE_SOURCE_DIR}/doc/JSBSim.dox.in
    ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif()

################################################################################
# Build the Python module and the automated test infrastructure (needs Python  #
# and Cython)                                                                  #
################################################################################

option(BUILD_PYTHON_MODULE "Set to ON to build the Python module for JSBSim" ON)
option(INSTALL_PYTHON_MODULE "Set to ON to install the Python module for JSBSim" OFF)

if (BUILD_PYTHON_MODULE)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/python/CMakeModules)
  find_package(Cython)

  if (CYTHON_FOUND)
    find_package(PythonLibs)
    if (PYTHONLIBS_FOUND)
      enable_testing()
      add_subdirectory(tests)
      add_subdirectory(python)
    endif(PYTHONLIBS_FOUND)
  elseif(NOT PYTHONLIBS_FOUND)
    message(WARNING "JSBSim Python module and test suite will not be built")
  endif(CYTHON_FOUND)
endif(BUILD_PYTHON_MODULE)

################################################################################
# Build the unit tests (needs CxxTest)                                         #
################################################################################

find_package(CxxTest)

if (CXXTEST_FOUND)
  include_directories(${CMAKE_SOURCE_DIR}/src)
  enable_testing()
  add_subdirectory(tests/unit_tests)
endif(CXXTEST_FOUND)

################################################################################
# Packaging                                                                    #
################################################################################

find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/JSBSim.pc.in ${CMAKE_CURRENT_BINARY_DIR}/JSBSim.pc @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/JSBSim.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig COMPONENT devel)
endif()

# Extract the host architecture for the package name
if (UNIX)
  execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (NOT DEFINED LSB_RELEASE_EXECUTABLE)
    find_program(LSB_RELEASE_EXECUTABLE lsb_release)
    if(LSB_RELEASE_EXECUTABLE STREQUAL "LSB_RELEASE_EXECUTABLE-NOTFOUND")
      message("-- Could NOT find LSB_Release (missing: LSB_RELEASE_EXECUTABLE)")
    else()
      message("-- Found LSB_Release: ${LSB_RELEASE_EXECUTABLE}")
    endif()
  endif()

  if(LSB_RELEASE_EXECUTABLE STREQUAL "LSB_RELEASE_EXECUTABLE-NOTFOUND")
    set(LINUX_DISTRIB_NAME)
    set(LINUX_DISTRIB_RELEASE)
  else()
    execute_process(COMMAND ${LSB_RELEASE_EXECUTABLE} -i OUTPUT_VARIABLE DISTRIB_NAME)
    execute_process(COMMAND ${LSB_RELEASE_EXECUTABLE} -r OUTPUT_VARIABLE DISTRIB_RELEASE)
    get_tail(${DISTRIB_NAME} LINUX_DISTRIB_NAME "\t")
    get_tail(${DISTRIB_RELEASE} LINUX_DISTRIB_RELEASE "\t")
    string(TOLOWER ${LINUX_DISTRIB_NAME} TEMP_NAME)
    if (TEMP_NAME STREQUAL "fedora")
      set(LINUX_DISTRIB_NAME ".fc")
    elseif(TEMP_NAME STREQUAL "ubuntu")
      if(LINUX_DISTRIB_RELEASE STREQUAL "14.04")
        set(LINUX_DISTRIB_NAME "~trusty")
        set(LINUX_DISTRIB_RELEASE)
      elseif(LINUX_DISTRIB_RELEASE STREQUAL "16.04")
        set(LINUX_DISTRIB_NAME "~xenial")
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
# Debian/Ubuntu packaging                                                      #
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
# Red Hat packaging                                                            #
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
# Source packaging                                                             #
################################################################################

set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR}/"
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
